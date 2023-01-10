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

    char encoded[291];
    char encoded_response[1026];
    char inbox_message[1024];
    
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
    

    //Encode message with protocol
    prot_encode_sub_registration(argv[2], argv[3], encoded, sizeof(encoded));

    ssize_t wr = write(pipe, encoded, sizeof(encoded));
    if (wr == -1)
        return -1;


    // Create the session pipe (remove the old one if it was already created)
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERROR]: unlink(%s) failed: %s\n", argv[2],
            strerror(errno));
        exit(EXIT_FAILURE);
    }
    if(mkfifo(argv[2], 0666) == -1) {
        fprintf(stderr, "[ERROR]: Failed to create pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }


    int pipe_name = open(argv[2], O_RDONLY);
    if (pipe_name == -1) {
        fprintf(stderr, "[ERROR]: Failed to open newly created pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    ssize_t rd_resp = read(pipe_name, encoded, sizeof(encoded));

    prot_decode_message(inbox_message, encoded_response, sizeof(encoded_response));

    //Mostrar mensagens
    

    return -1;
}
