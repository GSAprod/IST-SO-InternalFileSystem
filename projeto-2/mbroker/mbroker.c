#include "logging.h"

void print_usage() {
    fprintf(stderr, "usage: mbroker <pipename>\n");
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    WARN("unimplemented"); // TODO: implement

    if(argc != 2) {
        print_usage();
        return -1;
    }
    return -1;
}
