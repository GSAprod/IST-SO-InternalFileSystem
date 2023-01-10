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

int register_pipe = -1, session_pipe = -1;

static void print_usage() {
    fprintf(stderr, "usage: pub <register_pipe_name> <pipe_name> <box_name>\n");
}

int signalHandler(int signal) {
    if(register_pipe != -1) {
        close(register_pipe);
    }
    if(session_pipe != -1) {
        close(session_pipe);
    }
    char str[33] = "[INFO] Received SIGINT. Exiting\n";
    write(stdout, str, sizeof(str));
    exit(0);
}

int main(int argc, char **argv) {

    char encoded[291];
    int x=0;        
    char message_to_write[1024];
    
    if(argc != 4) {
        print_usage();
        return -1;
    }

    if(signal(SIGINT, signalHandler) == SIG_ERR) {
        fprintf(stderr, "[ERROR]: Failed to attach sigHandler to process\n");
        exit(EXIT_FAILURE);
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
    register_pipe = open(argv[1], O_WRONLY);
    if (register_pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
        return -1;
    }

    //Encode message with protocol and write onto the register pipe
    prot_encode_pub_registration(argv[2], argv[3], encoded, sizeof(encoded));
    ssize_t wr = write(register_pipe, encoded, sizeof(encoded));
    close(register_pipe);
    if (wr == -1) {
        fprintf(stderr, "[ERROR]: Failed to write onto pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    // TODO: Fix problem
    session_pipe = open(argv[2], O_WRONLY);
    if (session_pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open newly created pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    //Read messages to write in box
    printf("inserir\n");
    memset(message_to_write, 0, sizeof(message_to_write));
    int scan = scanf("%s", message_to_write);
    while (scan != EOF) {
        x++;
        if (scan == -1)
            return -1;

        printf("palavra->%s\n", message_to_write);
    
        ssize_t session_pipe_wr = write(session_pipe, message_to_write, strlen(message_to_write));
        
        if (session_pipe_wr == -1) {
            if(errno == EPIPE) {
                printf("Pipe fechado pelo mbroker. A terminar.\n");
                break;
            } else {
                fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
                close(session_pipe);
                exit(EXIT_FAILURE);
            }
        }

        memset(message_to_write, 0, sizeof(message_to_write));
        scan = scanf("%s", message_to_write);
    }
    printf("fecha pipe\n");
    close(session_pipe);
    unlink(argv[2]);

    return 0;
}