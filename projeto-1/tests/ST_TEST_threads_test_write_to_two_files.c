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
#define APPEND_CONTENTS_SYMLINK ("ABC")
#define APPEND_CONTENTS_HARDLINK ("abc")
#define THREAD_NUMBER 2

typedef struct {
    ssize_t n_writes;
    char *path;
    char *text;
} write_args_t;

uint8_t const file_1_contents[] = "789789789789789789789789789789";
uint8_t const file_2_contents[] = "123123123123123123123123123123";

void *write_to_path_fn(void* arg) {
    write_args_t const *args = (write_args_t const *)arg;

    // For i iterations, each thread will open a file, write some content onto it and close it.
    for(size_t i = 0; i < args->n_writes; i++) {
        int f = tfs_open(args->path, TFS_O_APPEND);
        assert(f != -1);

        assert(tfs_write(f, args->text, strlen(args->text)) == strlen(args->text));

        assert(tfs_close(f) != -1);
    }
    return NULL;
}

void assert_contents_ok(char const *path, uint8_t const *contents) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(contents) - 1];
    assert(tfs_read(f, buffer, sizeof(buffer)) != -1);
    assert(memcmp(buffer, contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

int main() {
    char *file_path = "/f1";
    char *file_path_2 = "/f2";
    ssize_t total_ops = 10;
    pthread_t tid[THREAD_NUMBER];

    // Initialize tfs with default parameters
    tfs_params params = tfs_default_params();
    assert(tfs_init(&params) != -1);

    // Create two empty files, ready to be used by the threads
    int f1 = tfs_open(file_path, TFS_O_CREAT);
    assert(f1 != -1);
    int f2 = tfs_open(file_path_2, TFS_O_CREAT);
    assert(f2 != -1);
    assert(tfs_close(f1) != -1);
    assert(tfs_close(f2) != -1);

    // Create all arguments for the threads
    write_args_t thread_args_1 = { .n_writes = total_ops, .path = file_path, .text = "789"};
    write_args_t thread_args_2 = { .n_writes = total_ops, .path = file_path_2, .text = "123"};

    // Add the thread that writes onto the first file
    if (pthread_create(&tid[0], NULL, write_to_path_fn, (void *) &thread_args_1) != 0) {
        fprintf(stderr, "failed to create file writing thread: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Add the thread that writes onto the second file
    if (pthread_create(&tid[1], NULL, write_to_path_fn, (void *) &thread_args_2) != 0) {
        fprintf(stderr, "failed to create hardlink writing thread: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Wait for both threads to finish
    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    // Check if the contents of the files are the same as predicted.
    assert_contents_ok(file_path, file_1_contents);
    assert_contents_ok(file_path_2, file_2_contents);

    assert(tfs_destroy() != -1);
    
    printf("Successful test.\n");

    return 0;
}
