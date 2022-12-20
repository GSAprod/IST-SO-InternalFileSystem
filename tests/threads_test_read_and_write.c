#include "fs/operations.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#define APPEND_CONTENTS_FILE ("123")

typedef struct {
    ssize_t n_writes;
    char *path;
    char *text;
} write_args_t;

typedef struct {
    ssize_t n_reads;
    char *path;
} read_args_t;

uint8_t const file_contents[] = "123123123123123123123123123123";

void *write_to_path_fn(void* arg) {
    write_args_t const *args = (write_args_t const *)arg;

    for(ssize_t i = 0; i < args->n_writes; i++) {
        int f = tfs_open(args->path, TFS_O_CREAT | TFS_O_APPEND);
        assert(f != -1);

        assert(tfs_write(f, APPEND_CONTENTS_FILE, strlen(APPEND_CONTENTS_FILE)) != -1);

        assert(tfs_close(f) != -1);

    }
    return NULL;
}

void *read_from_path_fn(void* arg) {
    char buffer[600];
    read_args_t const *args = (read_args_t const *)arg;

    for (ssize_t i = 0; i < args->n_reads; i++)
    {
        int f = tfs_open(args->path, TFS_O_CREAT);
        assert(f != -1);
        int bytes_read = -1;
        memset(buffer, 0, sizeof(buffer));
        
        while(bytes_read) {
            bytes_read = (int) tfs_read(f, buffer, (size_t) sizeof(buffer) - 1);
            assert(bytes_read != -1);
        }
    }

    return NULL;
}

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents) - 1];
    assert(tfs_read(f, buffer, sizeof(buffer)) != -1);
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

int main() {
    char *file_path = "/f1";

    pthread_t tid[2];

    ssize_t total_ops = 10;

    // Initialize tfs with default parameters
    tfs_params params = tfs_default_params();
    assert(tfs_init(&params) != -1);

    // Create file with content
    int f1 = tfs_open(file_path, TFS_O_CREAT);
    assert(f1 != -1);
    assert(tfs_close(f1) != -1);

    // Create all arguments for the threads
    write_args_t write_args = { .n_writes = total_ops, .path = file_path, .text = APPEND_CONTENTS_FILE};
    read_args_t read_args = { .n_reads = total_ops, .path = file_path };

    // Add the thread that writes onto the file
    if (pthread_create(&tid[0], NULL, write_to_path_fn, (void *) &write_args) != 0) {
        fprintf(stderr, "failed to create file writing thread: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Add the thread that reads from the file
    if (pthread_create(&tid[1], NULL, read_from_path_fn, (void *) &read_args) != 0) {
        fprintf(stderr, "failed to create hardlink writing thread: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Wait for all threads to finish
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    // Check if the contents of the file are the same as predicted.
    assert_contents_ok(file_path);
    assert(tfs_destroy() != -1);

    printf("Successful test.\n");

    return 0;
}
