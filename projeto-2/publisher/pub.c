#include "logging.h"
#include "../utils/wire_protocol.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <signal.h>



static void print_usage() {
    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
}


int main(int argc, char **argv) {
    
    if(argc != 4) {
        print_usage();
        return -1;
    }

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
    
    //Register pipe
    int register_pipe = open(argv[1], O_WRONLY);
    if (register_pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
        return -1;
    }

    //Encode message with protocol and write onto the register pipe
    char encoded[291];
    prot_encode_pub_registration(argv[2], argv[3], encoded, sizeof(encoded));
    ssize_t wr = write(register_pipe, encoded, sizeof(encoded));
    close(register_pipe);
    if (wr == -1) {
        fprintf(stderr, "[ERROR]: Failed to write onto pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    int pipe_name = open(argv[2], O_WRONLY);
    if (pipe_name == -1) {
        fprintf(stderr, "[ERROR]: Failed to open newly created pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } 
    
    //Read messages to write in box
    int x=0;        
    char message_to_write[256];
    printf("inserir\n");
    while (x<3) {
        x++;
        memset(message_to_write, 0, sizeof(message_to_write));
        int scan = scanf("%s", message_to_write);
        if (scan == -1)
            return -1;

        printf("palavra->%s\n", message_to_write);
    
        ssize_t pipe_name_wr = write(pipe_name, message_to_write, strlen(message_to_write));
        
        if (pipe_name_wr == -1) {
            if(errno == EPIPE) {
                printf("Pipe fechado pelo mbroker. A terminar.\n");
                break;
            } else {
                fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    }
    printf("fecha pipe\n");
    close(pipe_name);
    unlink(argv[2]);

    return 0;
}