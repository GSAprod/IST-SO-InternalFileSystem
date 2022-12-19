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

typedef struct {
    int num;
    ssize_t n_writes;
    char *path;
    char *text;
} write_args_t;

uint8_t const file_1_contents[] = "789789789789789789789789789789";

void *write_to_path_fn(void* arg) {
    write_args_t const *args = (write_args_t const *)arg;

    size_t total_writes = 0;
    for(size_t i = 0; i < args->n_writes; i++) {
        int f = tfs_open(args->path, TFS_O_APPEND);
        assert(f != -1);

        assert(tfs_write(f, args->text, strlen(args->text)) == strlen(args->text));

        assert(tfs_close(f) != -1);

        total_writes++;

        printf("(%d) Thread wrote %zd times into path \"%s\"\n", args->num, total_writes, args->path);
    }
    printf("(%d) Thread stopped writing into path \"%s\"\n", args->num, args->path);
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
    } while(r == sizeof(buffer) - 1);

    printf("\n");
    tfs_close(fd);
}

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_1_contents)];
    ssize_t r = tfs_read(f, buffer, sizeof(buffer));
    printf("aqui - %s\n",buffer);
    assert(r == sizeof(buffer));
    assert(memcmp(buffer, file_1_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

int main() {
    char *file_path = "/f1";
    char *file_path_2 = "/f2";

    pthread_t tid[2];

    ssize_t total_ops = 10;

    tfs_init(NULL);

    // Create file with content
    int f1 = tfs_open(file_path, TFS_O_CREAT);
    assert(f1 != -1);
    int f2 = tfs_open(file_path_2, TFS_O_CREAT);
    assert(f2 != -1);
    assert(tfs_close(f1) != -1);
    assert(tfs_close(f2) != -1);

    // Create all arguments for the threads
    write_args_t thread_args_1 = { .num = 1, .n_writes = total_ops, .path = file_path, .text = "789"};
    write_args_t thread_args_2 = { .num = 2, .n_writes = total_ops, .path = file_path_2, .text = "123"};

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

    pthread_join(tid[0], NULL);
    pthread_join(tid[1], NULL);

    printf("FINISH!!!\n");
    print_file_contents(file_path);
    print_file_contents(file_path_2);

    assert_contents_ok(file_path);
    
    tfs_destroy();

    return 0;
}
