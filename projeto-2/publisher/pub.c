#include "logging.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>



static void print_usage() {
    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
}

int main(int argc, char **argv) {
    
    if(argc != 4) {
        print_usage();
        return -1;
    }

    if (unlink(argv[2]) != 0 && errno != ENOENT) {
    fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2],
            strerror(errno));
    exit(EXIT_FAILURE);
}
    
    //Register pipe
    int pipe = open(argv[1], O_WRONLY);
    
    
    //if (pipe == -1 || pipe_name == -1) {
    //    fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
    //}

    

    ssize_t wr = write(pipe, argv[2], strlen(argv[2]));
    if (wr == -1)
        return -1;

    
    int pipe_name = open(argv[2], O_RDONLY);

    //Perguntar se é espera ativa
    while(pipe_name == -1) {
        pipe_name = open(argv[2], O_RDONLY);
    }

    if(pipe_name == -1)
        printf("erro");

    char buffer[256];

    ssize_t rd = read(pipe_name, &buffer, 5);
    if (rd == -1)
        printf("erro2");

    printf("mensagem do mbroker->%s\n", buffer);

    return 0;
}