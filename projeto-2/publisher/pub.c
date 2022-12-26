#include "logging.h"

static void print_usage() {
    fprintf(stderr, "usage: pub <register_pipe_name> <box_name>\n");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    WARN("unimplemented"); // TODO: implement

    if(argc != 3) {
        print_usage();
        return -1;
    }
    return -1;
}