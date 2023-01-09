#include "logging.h"
#include "../utils/wire_protocol.h"
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

    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return -1;
    }
    
    char encoded[291];

    //Encode message with protocol
    prot_encode_sub_registration(argv[2], argv[3], encoded, sizeof(encoded));

    ssize_t wr = write(pipe, encoded, sizeof(encoded));
    if (wr == -1)
        return -1;

    return -1;
}
