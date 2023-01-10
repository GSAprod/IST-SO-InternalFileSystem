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

typedef struct box {
    int subscribers;
    int publishers;
    char box_name[32];
} Box;

Box boxes[MAX_INBOXES];


void print_usage() {
    fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
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

    int boxIndex;
    //Verify if box exists
    for (int i = 0; i<MAX_INBOXES; i++) {
        if (strcmp(boxes[i].box_name, box_name) == 0) {
            // TODO: Experimentar com >= no caso de existir mais que um publisher
            if (boxes[i].publishers >= 0) {
                printf("ja existe um publisher ligado\n");
                unlink(pipe_name);
                return;
            }
            boxes[i].publishers++;
            boxIndex = i;
            break;
        }
        printf("caixa nao existe");
        unlink(pipe_name);
        return;
    }

    char message_to_write[256];
    ssize_t bytes_read = 1;
    
    //TO ASK: espera ativa?
    int session_pipe = open(pipe_name, O_RDONLY);
    if (session_pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return;
    }

    //Wait for write until pipe is closed
    while (true) {

        memset(message_to_write, 0, sizeof(message_to_write));
        bytes_read = read(session_pipe, &message_to_write, sizeof(message_to_write));
        if (bytes_read == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            return;
        } else if (bytes_read == 0) 
            break;
        printf("received: %s\n", message_to_write);
        write_in_box(box_name, message_to_write);
    }

    close(session_pipe);
    boxes[boxIndex].publishers--;
}


void create_box(char *box_name, char *pipe_name) {

    char encoded[1030];
    char error_message[1024];
    //0 if box is created, -1 is box is not created
    int return_code = 0;
    
    //Create session pipe
    mkfifo(pipe_name, 0666);

    int pipe = open(pipe_name, O_WRONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return;
    }


    //Verify if box already exists
    for (int i = 0; i<MAX_INBOXES; i++)
        if (strcmp(boxes[i].box_name, box_name) == 0) {
            return_code = -1;
            strcpy(error_message, "Error while creating box. Box not created.");
            prot_encode_inbox_creation_resp(return_code, error_message, encoded, sizeof(encoded));

            //Send responde to the create box request
            ssize_t wr = write(pipe, encoded, sizeof(encoded));
            if (wr == -1) {
                fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
            }
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
    pipe_name++;
    //TO DO: Criar pipe e enviar caixas para o manager

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    if (root_dir_inode == NULL) {
        fprintf(stderr, "[ERROR]: Failed to get root directory inode: %s\n", strerror(errno));
        return;
    }

    dir_entry_t *dir_entry = (dir_entry_t *)data_block_get(root_dir_inode->i_data_block);
    
    // Iterates over the directory entries looking for boxes
    char box_name[50];

    for (int i = 0; i < MAX_INBOXES; i++)
        if ((dir_entry[i].d_inumber != -1)) {

            strcpy(box_name, dir_entry[i].d_name);
            printf("%s\n", box_name);
        }
}


void remove_box(char *box_name, char *pipe_name) {

    char encoded[1030];
    char error_message[1024];
    //0 if box is removed, -1 is box is not removed
    int return_code = 0;

    //Create session pipe
    mkfifo(pipe_name, 0666);
    

    int pipe = open(pipe_name, O_WRONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return;
    }


    //If the box is not removed, create error message
    if (tfs_unlink(box_name) == -1) {
        return_code = -1;
        strcpy(error_message, "Error while removing box.");
    }
    
    prot_encode_inbox_creation_resp(return_code, error_message, encoded, sizeof(encoded));

    //Send responde to the create box request
    ssize_t wr = write(pipe, encoded, sizeof(encoded));
    if (wr == -1)
        return;
}


void connect_subscriber(char *box_name, char *pipe_name) {

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

    char message[256];

    ssize_t bytes_read = tfs_read(box, message, sizeof(message));
    
    //Empty box
    if (bytes_read == 0) {
        printf("Sem mensagens");
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
        char pipe_name[256];
        char box_name[32];

        printf("buffer->%s\n", buffer);
        printf("code->%d\n", code);
    
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
