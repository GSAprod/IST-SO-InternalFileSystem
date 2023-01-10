#include "logging.h"
#include "../fs/operations.h"
#include "../fs/state.h"
#include "../utils/wire_protocol.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>


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


void print_usage() {
    fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
}


void signalhandler(int sig) {
    (void)sig;
    
    tfs_destroy();
    printf("Processo encerrado com sucesso\n");
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


void write_in_box(char *box_name, char *message) {

    //if message is empty, don't write
    if (strlen(message) == 0){
        printf("Empty message\n");    
        return;
    }

    int box = tfs_open(box_name, TFS_O_APPEND);

    if (tfs_write(box, message, strlen(message)+1) == -1) {
        fprintf(stderr, "[ERROR]: Failed to write message: %s\n", strerror(errno));
    }

    tfs_close(box);
}


void connect_publisher(char *pipe_name, char *box_name) {

    char message_to_write[TFS_BLOCK_SIZE];

    //Verify if box exists
    int box_index = get_box_index(box_name);

    if (box_index == -1) {
        printf("box doesn't exist\n");
        unlink(pipe_name);
        return;
    } else {
        if (boxes[box_index].publishers > 0) {
            printf("There is already a publisher connected\n");
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
        ssize_t bytes_read = read(session_pipe, &message_to_write, sizeof(message_to_write));
        if (bytes_read == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            return;
        } else if (bytes_read == 0) 
            break;
        printf("text: %s\n", message_to_write);
        write_in_box(box_name, message_to_write);
    }

    close(session_pipe);
    boxes[box_index].publishers--;
}


void create_box(char *box_name, char *pipe_name) {

    char encoded[1030];
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


    int create_box = tfs_open(box_name, TFS_O_CREAT);
    
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
}


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


            //TO ASK: Está correto contar os \0?


            //Get size of box (bytes written)
            int box = tfs_open(boxes[i].box_name, 0);
            if (box == -1) {
                fprintf(stderr, "[ERROR]: Failed to open box: %s\n", strerror(errno));
                return;
            }

            ssize_t box_size = tfs_read(box, inbox_message, sizeof(inbox_message));
            tfs_close(box);

            //Encode message to respond to list request
            printf("last: %d\n", last);
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


void remove_box(char *box_name, char *pipe_name) {

    char encoded[1030];
    char error_message[ERROR_MESSAGE_SIZE];
    //0 if box is removed, -1 is box is not removed
    int return_code = 0;
    

    int pipe = open(pipe_name, O_WRONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return;
    }


    //If the box is not removed, create error message
    if (tfs_unlink(box_name) == -1) {

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
}


void connect_subscriber(char *box_name, char *pipe_name) {

    char message[TFS_BLOCK_SIZE];

    //Verify if box exists
    for (int i = 0; i<MAX_INBOXES; i++) {
        if (strcmp(boxes[i].box_name, box_name) == 0) {
            boxes[i].subscribers++;
            break;
        }
        printf("Box doesn't exist\n");
        return;
    }

    pipe_name++;
    //TO DO: criar pipe para enviar as mensagens lidas ao subscriber
    //TO DO: Contar mensagens e mostrar ao detetar signal.

    int box = tfs_open(box_name, 0);

    ssize_t bytes_read = tfs_read(box, message, sizeof(message));
    
    //Empty box
    if (bytes_read == 0) {
        printf("No messages");
        tfs_close(box);
        return;
    }
    //Failed to read from box
    if (bytes_read == -1) {
        fprintf(stderr, "[ERROR]: Failed to read message: %s\n", strerror(errno));
        tfs_close(box);
        return;
    }

    for (int i = 0; i < bytes_read - 1; i++) {
        if (message[i] == '\0') {
            if(message[i] + 1 == '\0') {
                break;
            }
            else {
                message[i] = '\n';
            }
        }
    }
    message[bytes_read - 1] = '\0';

    printf("Messages:\n%s\n", message);

    while (true) {
        
    }

    tfs_close(box);
}


int main(int argc, char **argv) {

    if(argc != 2) {
        print_usage();
        return -1;
    }

    if (tfs_init(NULL) == -1) {
        fprintf(stderr, "[ERROR]: Failed to init tfs: %s\n", strerror(errno));
        return -1;
    }

    signal(SIGINT, &signalhandler);

    if (unlink(argv[1]) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[1],
            strerror(errno));
    exit(EXIT_FAILURE);
}
    
    mkfifo(argv[1], 0666);
    
    int pipe = open(argv[1], O_RDONLY);

    int pipewait = open(argv[1], O_WRONLY);
    if (pipewait == -1)
        return -1;

    char buffer[291];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t rd = read(pipe, &buffer, 291);
        if (rd == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            return -1;
        }

        int code = buffer[0];
        char pipe_name[SESSION_PIPE_NAME_SIZE];
        char box_name[BOX_NAME_SIZE];

    
        switch (code) {
            case 1:
                prot_decode_registrations(pipe_name, box_name, buffer, sizeof(buffer));
                connect_publisher(pipe_name, box_name);
                break;
            case 2:
                prot_decode_registrations(pipe_name, box_name, buffer, sizeof(buffer));
                connect_subscriber(box_name, pipe_name);
                break;
            case 3:
                prot_decode_registrations(pipe_name, box_name, buffer, sizeof(buffer));
                create_box(box_name, pipe_name);
                break;
            case 5:
                prot_decode_registrations(pipe_name, box_name, buffer, sizeof(buffer));
                remove_box(box_name, pipe_name);
                break;
            case 7:
                prot_decode_inbox_listing_req(pipe_name, buffer, sizeof(buffer));
                list_boxes(pipe_name);
                break;
            default:
                printf("Invalid code\n");
        }
        printf("pipe->%s\n", pipe_name);
        printf("box->%s\n", box_name);
    }

    return -1;
}
