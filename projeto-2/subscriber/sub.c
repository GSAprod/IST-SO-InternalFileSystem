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
    //Session pipe
    int pipe_name = open(argv[2], O_WRONLY);
    
    if (pipe == -1 || pipe_name == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
    }

    ssize_t wr = write(pipe, argv[2], strlen(argv[2]));
    if (wr == -1)
        return -1;

    return -1;
}
