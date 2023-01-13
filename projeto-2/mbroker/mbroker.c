#include "logging.h"
#include "../fs/operations.h"
#include "../fs/state.h"
#include "../utils/wire_protocol.h"
#include "../producer-consumer/producer-consumer.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>


#define MAX_INBOXES 1024/sizeof(dir_entry_t)
#define BOX_NAME_SIZE 32
#define SESSION_PIPE_NAME_SIZE 256
#define ERROR_MESSAGE_SIZE 1024
#define TFS_BLOCK_SIZE 1024


typedef struct box {
    int subscribers;
    int publishers;
    char box_name[BOX_NAME_SIZE];
} Box;

Box boxes[MAX_INBOXES];
int active_boxes = 0;

pc_queue_t pc_queue;

pthread_cond_t write_cond;

void print_usage() {
    fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
}


void signalhandler(int sig) {
    (void)sig;
    
    tfs_destroy();
    pcq_destroy(&pc_queue);
    //pthread_cond_destroy(&write_cond);
    printf("\n[INFO]: CTRL+C. Process closed successfully\n");
    exit(EXIT_SUCCESS);
}


//Returns the box index, or -1 if it doens't exist
int get_box_index(char *box_name) {

    for (int i = 0; i<MAX_INBOXES; i++) {
        if (strcmp(boxes[i].box_name, box_name) == 0) {
            return i;
        }
    }

    return -1;
}



/****************************** Write in a box *******************************/
void write_in_box(char *box_name, char *message) {

    //if message is empty, don't write
    if (strlen(message) == 0){
        return;
    }

    int box = tfs_open(box_name, TFS_O_APPEND);

    if (tfs_write(box, message, strlen(message)+1) == -1) {
        fprintf(stderr, "[ERROR]: Failed to write message: %s\n", strerror(errno));
    }

    tfs_close(box);
}
/*****************************************************************************/



/************************* Connect publisher to box **************************/
void connect_publisher(char *box_name, char *pipe_name) {

    char message_to_write[TFS_BLOCK_SIZE];
    char box_path[BOX_NAME_SIZE];
    char encoded_message[1026];

    //Verify if box exists
    int box_index = get_box_index(box_name);

    if (box_index == -1) {
        int p = open(pipe_name, O_RDONLY);
        close(p);
        unlink(pipe_name);
        return;
    } else {
        if (boxes[box_index].publishers >= 1) {
            printf("There is already a publisher connected\n");
            int p = open(pipe_name, O_RDONLY);
            close(p);
            unlink(pipe_name);
            return;
        }
        boxes[box_index].publishers++;
    }


    int session_pipe = open(pipe_name, O_RDONLY);
    if (session_pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return;
    }

    //Wait for write until pipe is closed
    while (true) {

        memset(message_to_write, 0, sizeof(message_to_write));
        ssize_t bytes_read = read(session_pipe, &encoded_message, sizeof(encoded_message));
        if (bytes_read == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            return;
        } else if (bytes_read == 0) 
            break;
        
        prot_decode_message(message_to_write, encoded_message, sizeof(encoded_message));

        strcpy(box_path, "/");
        strcat(box_path, box_name);

        write_in_box(box_path, message_to_write);
        pthread_cond_broadcast(&write_cond);
    }

    close(session_pipe);
    boxes[box_index].publishers--;
}
/*****************************************************************************/



/************************ Connect subscriber to a box ************************/
void connect_subscriber(char *box_name, char *pipe_name) {

    signal(SIGPIPE, SIG_IGN);

    char buffer[TFS_BLOCK_SIZE];
    char box_path[BOX_NAME_SIZE];
    char encoded[1026];

    pthread_mutex_t lock;

    int box_index = get_box_index(box_name);
    if (box_index == -1) {
        int p = open(pipe_name, O_WRONLY);
        close(p);
        unlink(pipe_name);
        return;
    }

    strcpy(box_path, "/");
    strcat(box_path, box_name);
    
    int box = tfs_open(box_path, 0);

    // Open the pipe to start communication
    int pipe = open(pipe_name, O_WRONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        tfs_close(box);
        return;
    }

    pthread_mutex_init(&lock, NULL);
    int able_to_read = 1;
    while (able_to_read) { 
        ssize_t bytes_read = tfs_read(box, buffer, sizeof(buffer));

        pthread_mutex_lock(&lock);
        
        //Empty box
        if (bytes_read == 0) {
            pthread_cond_wait(&write_cond, &lock);
            pthread_mutex_unlock(&lock);
            continue;
        }
        //Failed to read from box
        if (bytes_read == -1) {
            fprintf(stderr, "[ERROR]: Failed to read message: %s\n", strerror(errno));
            break;
        }

        pthread_mutex_unlock(&lock);
        //First read to a box. Do this to get all messages written before
        for (int i = 0; i < bytes_read - 1; i++) {
            if (buffer[i] == '\0') {
                if(buffer[i] + 1 == '\0') {
                    break;
                }
                else {
                    buffer[i] = '\n';
                }
            }
        }
        buffer[bytes_read - 1] = '\0';

        prot_encode_sub_receive_message(buffer, encoded, sizeof(encoded));
        
        ssize_t wr = write(pipe, encoded, sizeof(encoded));
        if (wr == -1) {
            able_to_read = 0;
        }
    }
    pthread_mutex_destroy(&lock);
    tfs_close(box);
    close(pipe);
}
/*****************************************************************************/



/******************************* Create a box ********************************/
void create_box(char *box_name, char *pipe_name) {

    char encoded[1030];
    char box_path[BOX_NAME_SIZE];
    char error_message[ERROR_MESSAGE_SIZE];
    //0 if box is created, -1 is box is not created
    int return_code = 0, box_index;

    int pipe = open(pipe_name, O_WRONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return;
    }


    box_index = get_box_index(box_name);
    if (box_index >= 0) {

        //Box already exists. Send error message
        return_code = -1;
        strcpy(error_message, "Error while creating box. Box not created.");
        prot_encode_inbox_creation_resp(return_code, error_message, encoded, sizeof(encoded));
        
        //Send responde to the create box request
        ssize_t wr = write(pipe, encoded, sizeof(encoded));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
        }
        close(pipe);
        return;
    }

    strcpy(box_path, "/");
    strcat(box_path, box_name);


    int create_box = tfs_open(box_path, TFS_O_CREAT);
    
    //If the box is not created, create error message
    if (create_box == -1){
        return_code = -1;
        strcpy(error_message, "Error while creating box. Box not created.");
    }

    //Add created box to the list
    for (int i = 0; i<MAX_INBOXES; i++)
        if (strlen(boxes[i].box_name) == 0) {
            strcpy(boxes[i].box_name, box_name);
            boxes[i].publishers = 0;
            boxes[i].subscribers = 0;
            active_boxes++;
            break;
        }

    prot_encode_inbox_creation_resp(return_code, error_message, encoded, sizeof(encoded));

    //Send responde to the create box request
    ssize_t wr = write(pipe, encoded, sizeof(encoded));
    if (wr == -1) {
        fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
        return;
    }

    tfs_close(create_box);

    fprintf(stdout, "OK\n");
}
/*****************************************************************************/



/******************************* Remove a box ********************************/
void remove_box(char *box_name, char *pipe_name) {

    char encoded[1030];
    char box_path[BOX_NAME_SIZE];
    char error_message[ERROR_MESSAGE_SIZE];
    //0 if box is removed, -1 is box is not removed
    int return_code = 0;
    

    int pipe = open(pipe_name, O_WRONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return;
    }

    strcpy(box_path, "/");
    strcat(box_path, box_name);

    //If the box is not removed, create error message
    if (tfs_unlink(box_path) == -1) {

        return_code = -1;
        strcpy(error_message, "Error while removing box.");
    } else {

        //Remove from box list
        int box_index = get_box_index(box_name);
        memset(boxes[box_index].box_name, 0, sizeof(boxes[box_index].box_name));
        boxes[box_index].publishers = 0;
        boxes[box_index].subscribers = 0;
        active_boxes--;
    }
    
    prot_encode_inbox_creation_resp(return_code, error_message, encoded, sizeof(encoded));

    //Send responde to the create box request
    ssize_t wr = write(pipe, encoded, sizeof(encoded));
    if (wr == -1)
        return;

    fprintf(stdout, "OK\n");
}
/*****************************************************************************/



/******************************** List boxes *********************************/
void list_boxes(char *pipe_name) {

    int boxes_listed = 0;
    __int8_t last = 0;
    char box_name[BOX_NAME_SIZE];
    char encoded[63];
    char inbox_message[TFS_BLOCK_SIZE];
    
    
    int pipe = open(pipe_name, O_WRONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return;
    }

    if (active_boxes == 0) {
        last = 1;
        memset(box_name, '\0', sizeof(box_name));
        prot_encode_inbox_listing_resp(last, box_name, 0, 0, 0, encoded, sizeof(encoded));
        
        //Send responde to the list boxes request
        ssize_t wr = write(pipe, encoded, sizeof(encoded));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
        }
        return;
    }

    for (int i = 0; i < MAX_INBOXES; i++)
        if (strlen(boxes[i].box_name) > 0) {
            if (boxes_listed + 1 == active_boxes) {
                last = 1;
            }

            memset(box_name, '\0', sizeof(box_name));
            strcpy(box_name, "/");
            strcat(box_name, boxes[i].box_name);

            //Get size of box (bytes written)
            int box = tfs_open(box_name, 0);
            if (box == -1) {
                fprintf(stderr, "[ERROR]: Failed to open box: %s\n", strerror(errno));
                return;
            }

            ssize_t box_size = tfs_read(box, inbox_message, sizeof(inbox_message));
            tfs_close(box);

            //Encode message to respond to list request
            prot_encode_inbox_listing_resp(last, boxes[i].box_name, box_size,
            boxes[i].subscribers, boxes[i].publishers, encoded, sizeof(encoded));

            //Send responde to the list boxes request
            ssize_t wr = write(pipe, encoded, sizeof(encoded));
            if (wr == -1) {
                fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
                return;
            }
            boxes_listed++;
            if (boxes_listed == active_boxes)
                return;
        }
}
/*****************************************************************************/



/****************************** Thread Manager *******************************/
void *thread_manager() {

    while (true) {
        char encoded[1026];
        memcpy(encoded, (char*) pcq_dequeue(&pc_queue), sizeof(encoded));

        int code = encoded[0];
        char pipe_name[SESSION_PIPE_NAME_SIZE];
        char box_name[BOX_NAME_SIZE];
        
        
        printf("element: %s\n", encoded);
        printf("code: %d\n", encoded[0]);
        
        pthread_cond_broadcast(&write_cond);

        switch (code) {
            case 1:
                prot_decode_registrations(pipe_name, box_name, encoded, sizeof(encoded));
                    connect_publisher(box_name, pipe_name);
                break;
            case 2:
                prot_decode_registrations(pipe_name, box_name, encoded, sizeof(encoded));
                connect_subscriber(box_name, pipe_name);
                break;
            case 3:
                prot_decode_registrations(pipe_name, box_name, encoded, sizeof(encoded));
                create_box(box_name, pipe_name);
                break;
            case 5:
                prot_decode_registrations(pipe_name, box_name, encoded, sizeof(encoded));
                remove_box(box_name, pipe_name);
                break;
            case 7:
                prot_decode_inbox_listing_req(pipe_name, encoded, sizeof(encoded));
                list_boxes(pipe_name);
                break;
            default:
                printf("Invalid code\n");
        }
    }

    return NULL;
}
/*****************************************************************************/



/*********************************** Main ************************************/
int main(int argc, char **argv) {

    if(argc != 3) {
        print_usage();
        return -1;
    }

    if (tfs_init(NULL) == -1) {
        fprintf(stderr, "[ERROR]: Failed to init tfs: %s\n", strerror(errno));
        return -1;
    }

    signal(SIGINT, &signalhandler);

    int max_sessions;
    max_sessions = atoi(argv[2]);

    /* Initialize the threads list and the queue */
    pthread_t threads[max_sessions];
    pcq_create(&pc_queue, (size_t) max_sessions);

    pthread_cond_init(&write_cond, NULL);

    for (int i = 0; i < max_sessions; i++) {
        pthread_create(&threads[i], NULL, thread_manager, NULL);
    }


    signal(SIGINT, &signalhandler);

    if (unlink(argv[1]) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
            strerror(errno));
    exit(EXIT_FAILURE);
}
    
    if(mkfifo(argv[1], 0666) == -1) {
        fprintf(stderr, "[ERROR]: Failed to create pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    int pipe = open(argv[1], O_RDONLY);

    int pipewait = open(argv[1], O_WRONLY);
    if (pipewait == -1)
        return -1;

    char encoded[291];

    while (true) {
        memset(encoded, 0, sizeof(encoded));
        
        ssize_t rd = read(pipe, &encoded, 291);
        if (rd == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            return -1;
        }

        pcq_enqueue(&pc_queue, encoded);
    }

    return -1;
}
/*****************************************************************************/