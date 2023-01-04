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

void print_usage() {
    fprintf(stderr, "usage: mbroker <pipename>\n");
}

void create_box(char *box_name) {
    int create_box = tfs_open(box_name, O_CREAT);
    if (create_box == -1) {
        fprintf(stderr, "[ERROR]: Failed to create box: %s\n", strerror(errno));
        return -1;
    }

    return 0;
}

void lista_caixas() {
    
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    if(argc != 2) {
        print_usage();
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
        if (strcmp(buffer, "cria caixa\n") == 0)
            create_box("caixa");
        printf("%s", buffer);
    }
    
    

    WARN("unimplemented"); // TODO: implement

    return -1;
}
