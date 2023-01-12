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

void print_usage() {
    fprintf(stderr, "usage: sub <register_session_pipe> <session_pipe> <box_name>\n");
}


void sigIntHandler(int signal) {
    (void)signal;
    if(register_pipe != -1)
        close(register_pipe);
    if(session_pipe != -1)
        close(session_pipe);
    
    puts("\n[INFO]: CTRL+C. Process closed successfully");

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


    //Decode response from mbroker
    ssize_t rd_resp = read(session_pipe, encoded_response, sizeof(encoded_response));
    if (rd_resp == -1) {
        fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else if (rd_resp == 0) {
        puts("Session pipe closed. Exiting");
        close(session_pipe);
        close(register_pipe);
        exit(0);
    }

    prot_decode_message(inbox_message, encoded_response, sizeof(encoded_response));

    if (strlen(inbox_message) > 0)
        fprintf(stdout, "%s\n", inbox_message);

    close(register_pipe);
    close(session_pipe);
    unlink(argv[2]);
    
    return 0;
}
