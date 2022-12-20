#include "fs/operations.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>


#define THREAD_NUMBER 20
#define PATH "/f1"

uint8_t const file_contents[] = "Text to write in file that will be readed by multiple threads";


/* This test fills a file, and then attempts to read from it simultaneously
 in multiple threads, comparing the content with the original source. */

void write_fn() {

    int f = tfs_open(PATH, TFS_O_CREAT);
    assert(f != -1);

    /* write the contents of the file */
    assert(tfs_write(f, file_contents, sizeof(file_contents)) != -1);

    assert(tfs_close(f) == 0);
}

void *read_fn(void *input) {
    (void)input; // ignore parameter


    char buffer[sizeof(file_contents)];

    int f = tfs_open(PATH, 0);
    assert(f != -1);

    assert(tfs_read(f, buffer, sizeof(file_contents)) != -1);
    
    assert(memcmp(file_contents, buffer, sizeof(file_contents)) == 0);

    assert(tfs_close(f) == 0);

    return NULL;
}

int main() {

    // Initialize tfs with default parameters
    tfs_params params = tfs_default_params();
    assert(tfs_init(&params) != -1);

    pthread_t tid[THREAD_NUMBER];

    write_fn();
    for (int i = 0; i < THREAD_NUMBER; i++) {
        assert(pthread_create(&tid[i], NULL, read_fn, NULL) == 0);
    }

    for (int i = 0; i < THREAD_NUMBER; i++) {
        assert(pthread_join(tid[i], NULL) == 0);
    }

    assert(tfs_destroy() == 0);
    printf("Successful test.\n");

    return 0;
}