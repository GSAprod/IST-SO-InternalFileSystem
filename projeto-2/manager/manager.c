#include "logging.h"
#include "../utils/wire_protocol.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdbool.h>


static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> <pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> <pipe_name> list\n");
}

int main(int argc, char **argv) {

    switch(argc) {
        case 4:
            if (strcmp(argv[3], "list") != 0) {
                print_usage();
                exit(EXIT_FAILURE);
            }
            break;
        case 5:
            if (strcmp(argv[3], "create") != 0 && strcmp(argv[3], "remove") != 0) {
                print_usage();
                exit(EXIT_FAILURE);
            }
            break;
        default:
            print_usage();
            exit(EXIT_FAILURE);
    }

    if (!strcmp(argv[0], "manager")) {
        print_usage();
        exit(EXIT_FAILURE);
    }

    //Open register pipe
    int pipe = open(argv[1], O_WRONLY);
    
    char encoded[291];

    // Create the session pipe (remove the old one if it was already created)
    if (unlink(argv[2]) != 0 && errno != ENOENT) {
        fprintf(stderr, "[ERROR]: unlink(%s) failed: %s\n", argv[2],
            strerror(errno));
        exit(EXIT_FAILURE);
    }

    //Create session pipe
    mkfifo(argv[2], 0666);


    if (strcmp(argv[3], "create") == 0) {

        char encoded_response[1030];
        char error_message[1024];
        int return_code;

        //Encode message with protocol
        prot_encode_inbox_creation_req(argv[2], argv[4], encoded, sizeof(encoded));

        ssize_t wr = write(pipe, encoded, sizeof(encoded));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write to pipe: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        //Open and read session pipe
        int pipe_name = open(argv[2], O_RDONLY);

        if (pipe_name == -1) {
            fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }


        ssize_t rd_resp = read(pipe_name, &encoded_response, sizeof(encoded_response));
        if (rd_resp == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        //Decode and print error message
        prot_decode_inbox_response(&return_code, error_message, encoded_response, sizeof(encoded_response));
        
        //In case of error, print error message
        if (return_code == -1)
            fprintf(stdout, "ERROR %s\n", error_message);
    }



    if (strcmp(argv[3], "remove") == 0) {

        char encoded_response[1030];
        char error_message[1024];
        int return_code;

        //Encode message with protocol
        prot_encode_inbox_removal_req(argv[2], argv[4], encoded, sizeof(encoded));

        ssize_t wr = write(pipe, encoded, sizeof(encoded));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write to pipe: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }


        //Open and read session pipe
        int pipe_name = open(argv[2], O_RDONLY);

        if (pipe_name == -1) {
            fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }


        ssize_t rd_resp = read(pipe_name, &encoded_response, sizeof(encoded_response));
        if (rd_resp == -1) {
            fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        //Decode and print error message (if needed)
        prot_decode_inbox_response(&return_code, error_message, encoded_response, sizeof(encoded_response));
        
        //In case of error, print error message
        if (return_code == -1)
            fprintf(stdout, "ERROR %s\n", error_message);
    }



    if (strcmp(argv[3], "list") == 0) {

        char encoded_response[63];
        char box_name[32];
        __int8_t last;
        __int64_t box_size, n_publishers, n_subscribers;

        //Encode message with protocol
        prot_encode_inbox_listing_req(argv[2], encoded, sizeof(encoded));

        ssize_t wr = write(pipe, encoded, sizeof(encoded));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write to pipe: %s\n", strerror(errno));
            exit(EXIT_FAILURE);   
        }

        //Open and read session pipe
        int pipe_name = open(argv[2], O_RDONLY);

        if (pipe_name == -1) {
            fprintf(stderr, "[ERROR]: Failed to open pipe: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }


        while (last != 1) {
        
            //Read encoded message
            ssize_t rd_resp = read(pipe_name, &encoded_response, sizeof(encoded_response));
            if (rd_resp == -1) {
                fprintf(stderr, "[ERROR]: Failed to read from pipe: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

            //Decode and print parameters
            prot_decode_inbox_listing_resp(&last, box_name, &box_size, &n_publishers,
                &n_subscribers, encoded_response, sizeof(encoded_response));

            if (last == 1 && strlen(box_name) == 0) {
                fprintf(stdout, "NO BOXES FOUND\n");
                break;
            }

            fprintf(stdout, "%s %zu %zu %zu\n", box_name, box_size, n_publishers, n_subscribers);
        }
    }

    //Delete pipe. No longer needed
    unlink(argv[2]);
    close(pipe);
    
    return 0;
}
