#include "logging.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

static void print_usage() {
    fprintf(stderr, "usage: \n"
                    "   manager <register_pipe_name> create <box_name>\n"
                    "   manager <register_pipe_name> remove <box_name>\n"
                    "   manager <register_pipe_name> list\n");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    switch(argc) {
        case 3:
            if (strcmp(argv[2], "list") != 0) {
                print_usage();
                return -1;
            }
            break;
        case 4:
            if (strcmp(argv[2], "create") != 0 && strcmp(argv[2], "remove") != 0) {
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

    int pipe = open(argv[1], O_WRONLY);

    if (strcmp(argv[2], "create") == 0) {
        ssize_t wr = write(pipe, argv[3], strlen(argv[3]));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write to pipe: %s\n", strerror(errno));
            return -1;    
        }
        
    }

    if (strcmp(argv[2], "remove") == 0) {
        ssize_t wr = write(pipe, "remove caixa", strlen("remove caixa"));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write to pipe: %s\n", strerror(errno));
            return -1;    
        }
    }

    if (strcmp(argv[2], "list") == 0) {
        ssize_t wr = write(pipe, "lista caixas", strlen("lista caixas"));
        if (wr == -1) {
            fprintf(stderr, "[ERROR]: Failed to write to pipe: %s\n", strerror(errno));
            return -1;    
        }
    }

    WARN("unimplemented"); // TODO: implement

    
    return -1;
}
