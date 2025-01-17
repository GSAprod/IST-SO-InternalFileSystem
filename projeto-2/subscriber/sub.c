#include "logging.h"
#include "../utils/wire_protocol.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>


int register_pipe = -1, session_pipe = -1;
char session_pipe_name[256];
int message_count = 0;

void print_usage() {
    fprintf(stderr, "usage: sub <register_session_pipe> <session_pipe> <box_name>\n");
}


void sigIntHandler(int signal) {
    (void)signal;
    if(register_pipe != -1)
        close(register_pipe);
    if(session_pipe != -1) {
        close(session_pipe);
        unlink(session_pipe_name);
    }
    
    printf("\nSignal interrupt. Received %d messages.\n", message_count);
    exit(0);
}



int main(int argc, char **argv) {

    char encoded[291];
    char encoded_response[1026];
    char inbox_message[1024];
    
    if(argc != 4) {
        print_usage();
        exit(EXIT_FAILURE);
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
    strcpy(session_pipe_name, argv[2]);

    //Register pipe
    register_pipe = open(argv[1], O_WRONLY);
    if (register_pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    

    //Encode message with protocol
    prot_encode_sub_registration(argv[2], argv[3], encoded, sizeof(encoded));

    ssize_t wr = write(register_pipe, encoded, sizeof(encoded));
    if (wr == -1) {
        fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    //Session pipe
    session_pipe = open(argv[2], O_RDONLY);
    if (session_pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    message_count = 0;
    ssize_t rd_resp = read(session_pipe, encoded_response, sizeof(encoded_response));
    while (rd_resp > 0) {
        //Check if pipe is still open
        int aux = open(argv[2], O_RDONLY);
        if (aux == -1) {
            break;
        }

        prot_decode_message(inbox_message, encoded_response, sizeof(encoded_response));

        if (strlen(inbox_message) > 0) {
            fprintf(stdout, "%s\n", inbox_message);

            // Count the number of messages received
            for(int i = 0; i < strlen(inbox_message); i++)
                if(inbox_message[i] == '\n')
                    message_count++;
            
            message_count++;
        }
        rd_resp = read(session_pipe, encoded_response, sizeof(encoded_response));
    }
    //Decode response from mbroker
    if (rd_resp == -1) {
        fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
    } else {
        printf("Received %d messages", message_count);
    }

    close(register_pipe);
    close(session_pipe);
    unlink(argv[2]);
    
    return 0;
}
