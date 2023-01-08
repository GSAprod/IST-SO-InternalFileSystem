#include "logging.h"
#include "../fs/operations.h"
#include "../fs/state.h"
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdbool.h>

#define MAX_INBOXES 1024/sizeof(dir_entry_t)

void print_usage() {
    fprintf(stderr, "usage: mbroker <pipename> <max_sessions>\n");
}


void write_in_box(char *box_name, char *message) {

    //if message is empty, don't write
    if (strlen(message) == 0)
        return;

    int box = tfs_open(box_name, TFS_O_APPEND);

    if (tfs_write(box, message, strlen(message)+1) == -1) {
        fprintf(stderr, "[ERROR]: Failed to write message: %s\n", strerror(errno));
    }

    tfs_close(box);
    return;
}


void connect_publisher(char *pipe_name) {

    //Create session pipe
    mkfifo(pipe_name, 0666);
    
    //int pipe = open(pipe_name, O_RDONLY);

    int pipe = open(pipe_name, O_WRONLY);
    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return;
    }

    ssize_t wr = write(pipe, "request accepted", strlen("request accepted"));
    if (wr == -1)
        return;
    
    char message_to_write[256];
    
    //TO ASK: espera ativa?
    pipe = open(pipe_name, O_RDONLY);

    //Wait for write until pipe is closed
    while (pipe != -1) {
        
        ssize_t bytes_read = read(pipe, &message_to_write, sizeof(message_to_write));
        if (bytes_read > 0){
            write_in_box("/f1", message_to_write);
        }
        pipe = open(pipe_name, O_RDONLY);
    }
}


void create_box(char *box_name) {
    int create_box = tfs_open(box_name, TFS_O_CREAT);
    
    if (create_box == -1)
        fprintf(stderr, "[ERROR]: Failed to create box: %s\n", strerror(errno));

    tfs_close(create_box);
}


void list_boxes() {

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


void remove_box(char *box_name) {

    if (tfs_unlink(box_name) == -1)
        fprintf(stderr, "[ERROR]: Failed to remove box: %s\n", strerror(errno));
}


void read_from_box(char *box_name) {


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
            if(message[i] + 1 == '\0')
                break;
            else 
                message[i] = '\n';
        }
    }
    message[bytes_read - 1] = '\0';

    while (bytes_read > 0) {

        printf("Message->%s\n", message);
        
        bytes_read = tfs_read(box, message, sizeof(message));

        if (bytes_read == -1) {
            fprintf(stderr, "[ERROR]: Failed to read message: %s\n", strerror(errno));
            tfs_close(box);
            return;        
        }
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

    char buffer[256];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        
        ssize_t rd = read(pipe, &buffer, 256);
        if (rd == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            return -1;
        }

        printf("%s\n", buffer);

        if (strcmp(buffer, "/f1") == 0 || strcmp(buffer, "/f2") == 0 || strcmp(buffer, "/f3") == 0)
            create_box(buffer);

        if (strcmp(buffer, "lista caixas") == 0)
            list_boxes();

        if (strcmp(buffer, "remove caixa") == 0)
            remove_box("/f1");

        if (strcmp(buffer, "session_pipe") == 0) {
            printf("ok\n");
            connect_publisher(buffer);
        }

        if (strcmp(buffer, "le da caixa") == 0)
            read_from_box("/f1");
    }

    return -1;
}
