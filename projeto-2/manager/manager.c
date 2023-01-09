#include "logging.h"
#include "../utils/wire_protocol.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    switch(argc) {
        case 4:
            if (strcmp(argv[3], "list") != 0) {
                print_usage();
                return -1;
            }
            break;
        case 5:
            if (strcmp(argv[3], "create") != 0 && strcmp(argv[3], "remove") != 0) {
                print_usage();
                return -1;
            }
            break;
        default:
            print_usage();
            return -1;
    }

    if (!strcmp(argv[0], "manager")) {
        print_usage();
        return -1;
    }

    //Open register pipe
    int pipe = open(argv[1], O_WRONLY);
    
    char encoded[291];



    if (strcmp(argv[3], "create") == 0) {

        //Encode message with protocol
        prot_encode_inbox_creation_req(argv[2], argv[4], encoded, sizeof(encoded));

        ssize_t wr = write(pipe, encoded, sizeof(encoded));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write to pipe: %s\n", strerror(errno));
            return -1;    
        }

        //Open and read session pipe
        int pipe_name = open(argv[2], O_RDONLY);

        //TO ASK: espera ativa?
        while (pipe_name == -1) {
            pipe_name = open(argv[2], O_RDONLY);
        }

        char encoded_reponse[1030];

        ssize_t rd_resp = read(pipe_name, &encoded_reponse, sizeof(encoded_reponse));
        if (rd_resp == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            return -1;
        }

        //Decode and print error message
        char error_message[1024];
        int return_code;

        prot_decode_inbox_response(&return_code, error_message, encoded_reponse, sizeof(encoded_reponse));
        
        //In case of error, print error message
        if (return_code == -1)
            printf("%s\n", error_message);

        //Delete pipe. No longer needed
        unlink(argv[2]);
    }



    if (strcmp(argv[3], "remove") == 0) {

        //Encode message with protocol
        prot_encode_inbox_removal_req(argv[2], argv[4], encoded, sizeof(encoded));

        ssize_t wr = write(pipe, encoded, sizeof(encoded));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write to pipe: %s\n", strerror(errno));
            return -1;    
        }


        //Open and read session pipe
        int pipe_name = open(argv[2], O_RDONLY);

        //TO ASK: espera ativa?
        while (pipe_name == -1) {
            pipe_name = open(argv[2], O_RDONLY);
        }

        char encoded_reponse[1030];

        ssize_t rd_resp = read(pipe_name, &encoded_reponse, sizeof(encoded_reponse));
        if (rd_resp == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            return -1;
        }

        //Decode and print error message
        char error_message[1024];
        int return_code;

        prot_decode_inbox_response(&return_code, error_message, encoded_reponse, sizeof(encoded_reponse));
        
        //In case of error, print error message
        if (return_code == -1)
            printf("%s\n", error_message);

        //Delete pipe. No longer needed
        unlink(argv[2]);
    }



    if (strcmp(argv[3], "list") == 0) {

        //Encode message with protocol
        prot_encode_inbox_listing_req(argv[2], encoded, sizeof(encoded));

        //TO DO:Ler reposta do mbroker

        ssize_t wr = write(pipe, encoded, sizeof(encoded));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write to pipe: %s\n", strerror(errno));
            return -1;    
        }
    }

    WARN("unimplemented"); // TODO: implement

    
    return -1;
}
