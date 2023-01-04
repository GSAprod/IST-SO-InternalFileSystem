#include "logging.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


static void print_usage() {
    fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    WARN("unimplemented"); // TODO: implement

    if(argc != 3) {
        print_usage();
        return -1;
    }

    int pipe = open(argv[1], O_WRONLY);
    

    ssize_t wr = write(pipe, "ola\n", strlen("ola\n"));
    if (wr == -1)
        return -1;

    return -1;
}