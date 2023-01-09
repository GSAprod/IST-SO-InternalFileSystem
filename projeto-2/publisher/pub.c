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

    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERR]: unlink(%s) failed: %s\n", argv[2],
            strerror(errno));
        exit(EXIT_FAILURE);
    }
    
    //Register pipe
    int pipe = open(argv[1], O_WRONLY);

    if (pipe == -1) {
        fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
        return -1;
    }

    char encoded[291];

    //Encode message with protocol
    prot_encode_pub_registration(argv[2], argv[3], encoded, sizeof(encoded));

    ssize_t wr = write(pipe, encoded, sizeof(encoded));
    if (wr == -1)
        return -1;

    
    int pipe_name = open(argv[2], O_RDONLY);

    //TO ASK: espera ativa?
    while (pipe_name == -1) {
        pipe_name = open(argv[2], O_RDONLY);
    }

    char request[18];

    ssize_t rd = read(pipe_name, &request, sizeof(request));
    if (rd == -1) {
        fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
        return -1;
    }

    if (strcmp(request, "request denied") == 0) {
        close(pipe_name);
        return -1;
    }

    if (strcmp(request, "request accepted") == 0) {

        printf("aceite\n");

        pipe_name = open(argv[2], O_WRONLY);
        if (pipe_name == -1)
            printf("pipe nao aberto para escrita");
        
        
        //Read messages to write in box
        int x=0;        
        printf("inserir\n");
        while (x<3) {
            x++;
            char message_to_write[256];
            int scan = scanf("%s", message_to_write);
            if (scan == -1)
                return -1;

            printf("palavra->%s\n", message_to_write);
        
            ssize_t pipe_name_wr = write(pipe_name, message_to_write, strlen(message_to_write));
            
            if (pipe_name_wr == -1) {
                fprintf(stderr, "[ERROR]: Failed to write in pipe: %s\n", strerror(errno));
                return -1;
            }
        }
        printf("fecha pipe\n");
        unlink(argv[2]);
    }

    return 0;
}