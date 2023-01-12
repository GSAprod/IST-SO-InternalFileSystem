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

void sigIntHandler(int signal) {
    (void)signal;
    if(register_pipe != -1)
        close(register_pipe);
    if(session_pipe != -1)
        close(session_pipe);
    switch(signal) {
        case SIGINT:
            puts("\n[INFO]: CTRL+C. Process closed successfully");
            break;
        case SIGPIPE:
            puts("Session pipe closed. Exiting");
            break;
        default:
            break;
    }
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

    if(signal(SIGINT, &sigIntHandler) == SIG_ERR) {
        fprintf(stderr, "[ERROR]: Failed to attach sigHandler to process\n");
        exit(EXIT_FAILURE);
    }
    if(signal(SIGPIPE, &sigIntHandler) == SIG_ERR) {
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
        fprintf(stderr, "[ERROR]: Failed to open pipe kjsdfhsdkfj: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
        return -1;
    }

    //Encode message with protocol and write onto the register pipe
    prot_encode_pub_registration(argv[2], argv[3], encoded, sizeof(encoded));
    ssize_t wr = write(register_pipe, encoded, sizeof(encoded));
    close(register_pipe);
    if (wr == -1) {
        fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    session_pipe = open(argv[2], O_WRONLY);
    if (session_pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe 123: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }


    //If session pipe was closed by mbroker, this write will fail
    ssize_t pipe_test = write(session_pipe, "", 1);

    if (pipe_test == -1) {
        fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
        close(session_pipe);
        close(register_pipe);
        exit(EXIT_FAILURE);
    }



    //Read messages to write in box
    printf("Insert words to write in box (CTRL+D to stop):\n");
    int scan = scanf("%s", message_to_write);

    while (scan != EOF) {
        x++;
        if (scan == -1)
            return -1;
    
        ssize_t session_pipe_wr = write(session_pipe, message_to_write, strlen(message_to_write));

        if (session_pipe_wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        memset(message_to_write, 0, sizeof(message_to_write));
        scan = scanf("%s", message_to_write);
    }
    
    close(session_pipe);
    close(register_pipe);
    unlink(argv[2]);

    return 0;
}