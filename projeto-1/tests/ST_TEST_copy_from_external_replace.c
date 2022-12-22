#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {

    char *str_ext_fileA = 
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! BBB! "
        "BBB! BBB! BBB! BBB! BBB! ";
    char *path_srcA = "tests/file_to_copy_over512.txt";

    /* This file looks corrupted. This is used to test if unicode characters pass correctly */
    char *str_ext_fileB = 
        "CCC!";
    char *path_srcB = "tests/file_to_copy_alt.txt";

    char *path_copied_file = "/f1";

    char bufferA[600];

    assert(tfs_init(NULL) != -1);

    int f;
    ssize_t r;

    f = tfs_copy_from_external_fs(path_srcA, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    r = tfs_read(f, bufferA, sizeof(bufferA) - 1);
    assert(r == strlen(str_ext_fileA));
    assert(!memcmp(bufferA, str_ext_fileA, strlen(str_ext_fileA)));

    tfs_close(f);
    // Repeat the copy to the same file, but with a different source
    char bufferB[300];

    f = tfs_copy_from_external_fs(path_srcB, path_copied_file);
    assert(f != -1);

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);

    // Contents should be overwriten, not appended
    r = tfs_read(f, bufferB, sizeof(bufferB) - 1);
    assert(r == strlen(str_ext_fileB));
    assert(!memcmp(bufferB, str_ext_fileB, strlen(str_ext_fileB)));

    printf("Successful test.\n");

    return 0;
}
