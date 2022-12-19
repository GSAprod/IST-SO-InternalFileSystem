#include "fs/operations.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

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

void *write_to_path_fn(void* arg) {
    write_args_t const *args = (write_args_t const *)arg;

    ssize_t total_writes = 0;
    for(ssize_t i = 0; i < args->n_writes; i++) {
        int f = tfs_open(args->path, TFS_O_CREAT | TFS_O_APPEND);

        tfs_write(f, APPEND_CONTENTS_FILE, strlen(APPEND_CONTENTS_FILE));

        tfs_close(f);

        total_writes++;

        printf("Thread wrote %zd times into path \"%s\"\n", total_writes, args->path);
    }
    printf("Thread stopped writing into path \"%s\"\n", args->path);
    return NULL;
}

void *read_from_path_fn(void* arg) {
    char buffer[600];
    read_args_t const *args = (read_args_t const *)arg;

    printf("File contents:\n");

    for (ssize_t i = 0; i < args->n_reads; i++)
    {
        int count = 0;
        int f = tfs_open(args->path, TFS_O_CREAT);
        int bytes_read = -1;
        memset(buffer, 0, sizeof(buffer));
        
        while(bytes_read) {
            bytes_read = (int) tfs_read(f, buffer, (size_t) sizeof(buffer) - 1);
            printf("%s", buffer);
            count += bytes_read;
        }
        printf("\n---end of file (%d bytes read)---\n", count);
    }

    return NULL;
}

void print_file_contents(char *path) {
    char buffer[600];
    memset(buffer, 0, sizeof(buffer));

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

    pthread_t tid[2];

    ssize_t total_ops = 3;

    tfs_init(NULL);

    // Create file with content
    int f1 = tfs_open(file_path, TFS_O_CREAT);
    tfs_close(f1);

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

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    printf("FINISH!!!\n");
    print_file_contents(file_path);
    tfs_destroy();

    return 0;
}
