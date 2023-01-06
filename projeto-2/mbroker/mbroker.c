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
    fprintf(stderr, "usage: mbroker <pipename>\n");
}


int create_box(char *box_name) {
    int create_box = tfs_open(box_name, TFS_O_CREAT);
    
    if (create_box == -1) {
        fprintf(stderr, "[ERROR]: Failed to create box: %s\n", strerror(errno));
        tfs_close(create_box);
        return -1;
    }

    tfs_close(create_box);

    return 0;
}


int list_boxes() {

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    if (root_dir_inode == NULL) {
        fprintf(stderr, "[ERROR]: Failed to get root directory inode: %s\n", strerror(errno));
        return -1;    
    }

    dir_entry_t *dir_entry = (dir_entry_t *)data_block_get(root_dir_inode->i_data_block);
    
    // Iterates over the directory entries looking for boxes
    char box_name[50];
    for (int i = 0; i < MAX_INBOXES; i++)
        if ((dir_entry[i].d_inumber != -1)) {
            strcpy(box_name, dir_entry[i].d_name);
            printf("%s\n", box_name);
        }

    return 0;
}


int remove_box(char *box_name) {

    if (tfs_unlink(box_name) == -1) {
        fprintf(stderr, "[ERROR]: Failed to remove box: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}


int write_in_box(char *box_name, char *message) {

    //if message is empty, don't write
    if (strlen(message) == 0)
        return 0;

    int box = tfs_open(box_name, TFS_O_APPEND);

    if (tfs_write(box, message, strlen(message)+1) == -1) {
        fprintf(stderr, "[ERROR]: Failed to write message: %s\n", strerror(errno));
        tfs_close(box);
        return -1;
    }

    tfs_close(box);

    return 0;
}


int read_from_box(char *box_name) {

    int box = tfs_open(box_name, 0);

    char message[256];

    //ciclo ate string lida ser vazia
    if (tfs_read(box, message, sizeof(message)-1) == -1) {
        fprintf(stderr, "[ERROR]: Failed to reade message: %s\n", strerror(errno));
        return -1;
    }
    printf("message->%s\n", message);

    tfs_close(box);

    return 0;

}


int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if(argc != 2) {
        print_usage();
        return -1;
    }

    if (tfs_init(NULL) == -1) {
        fprintf(stderr, "[ERROR]: Failed to init tfs: %s\n", strerror(errno));
        return -1;
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

        if (strcmp(buffer, "escreve na caixa") == 0)
            write_in_box("/f1", "mensagem na caixa");

        if (strcmp(buffer, "le da caixa") == 0)
            read_from_box("/f1");
    }
    
    

    WARN("unimplemented"); // TODO: implement

    return -1;
}
