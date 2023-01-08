#include "logging.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

void print_usage() {
    fprintf(stderr, "usage: sub <register_pipe_name> <pipe_name> <box_name>\n");
}

int main(int argc, char **argv) {
    
    if(argc != 4) {
        print_usage();
        return -1;
    }

    //Register pipe
    int pipe = open(argv[1], O_WRONLY);
    
    ssize_t wr = write(pipe, "le da caixa", strlen("le da caixa"));
    if (wr == -1)
        return -1;

    return -1;
}
