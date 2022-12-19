#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "betterassert.h"

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;
    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name, inode_t *root_inode) {
    // assert that root_inode is the root directory
    if (root_inode != inode_get(ROOT_DIR_INUM))
        return -1;
    
    if (!valid_pathname(name)) {
        return -1;
    }

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inode, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");
    int inum = tfs_lookup(name, root_dir_inode);
    size_t offset;

    if (inum >= 0) {
        // The file already exists
        inode_t *inode = inode_get(inum);
        pthread_rwlock_rdlock(&(inode->rwlock_inode));
        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");
        
        if(inode->i_node_type == T_SYMLINK) {
            // Gets the name of the target from the data block of the symbolic link
            char buffer[inode->i_size];
            void *block = data_block_get(inode->i_data_block);
            pthread_rwlock_unlock(&(inode->rwlock_inode));
            memcpy(buffer, block, sizeof(buffer));

            return tfs_open(buffer, mode);
        }


        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
        pthread_rwlock_unlock(&(inode->rwlock_inode));

    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum == -1) {
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            
            return -1; // no space in directory
        }

        offset = 0;
    } else {
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
    // Checks if the target path name is valid
    if (!valid_pathname(target)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");

    int link_inum = inode_create(T_SYMLINK);
    if(link_inum == -1) {
        return -1;
    }
    inode_t *link_inode = inode_get(link_inum);
    pthread_rwlock_wrlock(&(link_inode->rwlock_inode));

    int link_data_block = data_block_alloc();
    if(link_data_block == -1) {
        pthread_rwlock_unlock(&(link_inode->rwlock_inode));
        return -1;
    }

    link_inode->i_data_block = link_data_block;
    void *block = data_block_get(link_data_block);

    int count = 1;
    while(target[count] != '\0') {
        count++;
    }

    memcpy(block, target, (size_t) count);
    link_inode->i_size = (size_t) count;

    //Add the new node (already linked with target) to the directory containing the link name
    if (add_dir_entry(root_dir_inode, link_name+1, link_inum) == -1) {
        pthread_rwlock_unlock(&(link_inode->rwlock_inode));
        return -1;
    }
        
    pthread_rwlock_unlock(&(link_inode->rwlock_inode));
    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    // Checks if the target path name is valid
    if (!valid_pathname(target)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    if (root_dir_inode == NULL)
            return -1;
    int inum = tfs_lookup(target, root_dir_inode);

    if (inum >= 0) {
        // The file already exists
        inode_t *inode = inode_get(inum);
        pthread_rwlock_wrlock(&(inode->rwlock_inode));

        if (inode == NULL) {
            pthread_rwlock_unlock(&(inode->rwlock_inode));
            return -1;
        }

        if (inode->i_node_type == T_SYMLINK) {
            pthread_rwlock_unlock(&(inode->rwlock_inode));
            return -1;
        }

        //Increase hard link counter
        inode->i_link_counter++;
        
        //Add the new node (already linked with target) to the directory containing the link name
        if (add_dir_entry(root_dir_inode, link_name+1, inum) == -1) {
            pthread_rwlock_unlock(&(inode->rwlock_inode));
            return -1;
        }
        pthread_rwlock_unlock(&(inode->rwlock_inode));
    }
    else
        return -1;

    return 0;
}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    pthread_rwlock_wrlock(&(file->rwlock_open_file_entry));
    if (file == NULL) {
        pthread_rwlock_unlock(&(file->rwlock_open_file_entry));
        return -1; // invalid fd
    }

    pthread_rwlock_unlock(&(file->rwlock_open_file_entry));
    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    pthread_rwlock_wrlock(&(file->rwlock_open_file_entry));

    if (file == NULL) {
        pthread_rwlock_unlock(&(file->rwlock_open_file_entry));
        return -1;
    }

    //  From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    pthread_rwlock_wrlock(&(inode->rwlock_inode));
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                pthread_rwlock_unlock(&(inode->rwlock_inode));
                pthread_rwlock_unlock(&(file->rwlock_open_file_entry));
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }

    }
    pthread_rwlock_unlock(&(inode->rwlock_inode));
    pthread_rwlock_unlock(&(file->rwlock_open_file_entry));

    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    pthread_rwlock_wrlock(&(file->rwlock_open_file_entry));

    if (file == NULL) {
        pthread_rwlock_unlock(&(file->rwlock_open_file_entry));
        return -1;
    }

    // From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");
    pthread_rwlock_wrlock(&(inode->rwlock_inode));

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }
    pthread_rwlock_unlock(&(inode->rwlock_inode));
    pthread_rwlock_unlock(&(file->rwlock_open_file_entry));

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    // Checks if the target path name is valid
    if (!valid_pathname(target)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    if (root_dir_inode == NULL)
            return -1;

    int inum = tfs_lookup(target, root_dir_inode);

    if (inum >= 0) {
        inode_t *inode = inode_get(inum);
        pthread_rwlock_rdlock(&(inode->rwlock_inode));

        if (inode->i_node_type == T_SYMLINK) {
            clear_dir_entry(root_dir_inode, target+1);
            if (inode->i_size > 0)
                data_block_free(inode->i_data_block);
            inode_delete(inum);
        }

        if (inode->i_node_type == T_FILE) {
            clear_dir_entry(root_dir_inode, target+1);
            inode->i_link_counter--;
            if (inode->i_link_counter == 0) 
                inode_delete(inum);
        }
        pthread_rwlock_unlock(&(inode->rwlock_inode));
    }
    else
        return -1;

    return 0;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    
    /* Opens the file from the external filesystem */
    FILE *externalFd = fopen(source_path, "r");
    if (externalFd == NULL) {
        return -1;
    }

    /* Creates a new file on the TecnicoFS API */
    int internalFd = tfs_open(dest_path, TFS_O_CREAT | TFS_O_TRUNC);
    if (internalFd == -1) {
        return -1;
    }

    /* Creates a new buffer that will be used for transferring the file's data */
    char buffer[128];
    memset(buffer, 0, sizeof(buffer));

    /* Read the content of the source file and write it onto the destination file */
    int bytes_written = -1, bytes_read = -1;

    while(bytes_written && bytes_read) {
        bytes_read = (int) fread(buffer, (size_t) sizeof(char), (size_t) sizeof(buffer) - 1, externalFd);
        bytes_written = (int) tfs_write(internalFd, buffer, (size_t) bytes_read);
    }


    if (fclose(externalFd) == EOF) return -1;
    return tfs_close(internalFd);
}