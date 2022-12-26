#include "logging.h"
#include <stdio.h>
#include <string.h>

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

    WARN("unimplemented"); // TODO: implement

    
    return -1;
}
