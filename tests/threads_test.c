#include "fs/operations.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#define APPEND_CONTENTS_FILE ("123")
#define APPEND_CONTENTS_SYMLINK ("ABC")
#define APPEND_CONTENTS_HARDLINK ("abc")

typedef struct {
    ssize_t n_writes;
    char *path;
    char *text;
} write_args_t;

void *write_to_path_fn(void* arg) {
    write_args_t const *args = (write_args_t const *)arg;

    size_t total_writes = 0;
    for(size_t i = 0; i < args->n_writes; i++) {
        int f = tfs_open(args->path, TFS_O_APPEND);

        tfs_write(f, APPEND_CONTENTS_FILE, strlen(APPEND_CONTENTS_FILE));

        tfs_close(f);

        total_writes++;

        printf("Thread wrote %zd times into path \"%s\"\n", total_writes, args->path);
    }
    printf("Thread stopped writing into path \"%s\"\n", args->path);
    return NULL;
}

void print_file_contents(char *path) {
    char buffer[600];

    printf("File contents: \n");

    int fd = tfs_open(path, TFS_O_CREAT);
    ssize_t r = -1;
    do {
        r = tfs_read(fd, buffer, sizeof(buffer) - 1);
        printf("%s", buffer);
    } while(r != 0);

    printf("\n");
    tfs_close(fd);
}

int main() {
    char *file_path = "/f1";
    char *hardlink_path = "/l1";
    char *symlink_path = "/l2";

    pthread_t tid[3];

    ssize_t total_ops = 3;

    tfs_init(NULL);

    // Create file with content
    int f1 = tfs_open(file_path, TFS_O_CREAT);
    tfs_close(f1);

    // Create a hard link on the file
    tfs_link(file_path, hardlink_path);

    // Create a soft link on the file
    tfs_sym_link(file_path, symlink_path);

    // Create all arguments for the threads
    write_args_t file_args = { .n_writes = total_ops, .path = file_path, .text = APPEND_CONTENTS_FILE};
    write_args_t hardlink_args = { .n_writes = total_ops, .path = hardlink_path, .text = APPEND_CONTENTS_HARDLINK};
    write_args_t symlink_args = { .n_writes = total_ops, .path = symlink_path, .text = APPEND_CONTENTS_SYMLINK};

    // Add the thread that writes onto the file
    if (pthread_create(&tid[0], NULL, write_to_path_fn, (void *) &file_args) != 0) {
        fprintf(stderr, "failed to create file writing thread: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Add the thread that writes onto the hard link
    if (pthread_create(&tid[1], NULL, write_to_path_fn, (void *) &hardlink_args) != 0) {
        fprintf(stderr, "failed to create hardlink writing thread: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Add the thread that writes onto the soft link
    if (pthread_create(&tid[2], NULL, write_to_path_fn, (void *) &symlink_args) != 0) {
        fprintf(stderr, "failed to create hardlink writing thread: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);
    pthread_join(tid[2], NULL);

    printf("FINISH!!!\n");
    print_file_contents(file_path);
    tfs_destroy();

    return 0;
}
