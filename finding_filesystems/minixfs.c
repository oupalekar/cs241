/**
 * finding_filesystems
 * CS 241 - Spring 2022
 */
#include "minixfs.h"
#include "minixfs_utils.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "fcntl.h"
#include <unistd.h>
#include "time.h"
#include "stdlib.h"
#include "math.h"

/**
 * Virtual paths:
 *  Add your new virtual endpoint to minixfs_virtual_path_names
 */
char *minixfs_virtual_path_names[] = {"info", /* add your paths here*/};

/**
 * Forward declaring block_info_string so that we can attach unused on it
 * This prevents a compiler warning if you haven't used it yet.
 *
 * This function generates the info string that the virtual endpoint info should
 * emit when read
 */
static char *block_info_string(ssize_t num_used_blocks) __attribute__((unused));
static char *block_info_string(ssize_t num_used_blocks) {
    char *block_string = NULL;
    ssize_t curr_free_blocks = DATA_NUMBER - num_used_blocks;
    asprintf(&block_string,
             "Free blocks: %zd\n"
             "Used blocks: %zd\n",
             curr_free_blocks, num_used_blocks);
    return block_string;
}

// Don't modify this line unless you know what you're doing
int minixfs_virtual_path_count =
    sizeof(minixfs_virtual_path_names) / sizeof(minixfs_virtual_path_names[0]);

int minixfs_chmod(file_system *fs, char *path, int new_permissions) {
    // Thar she blows!
    inode * inode = get_inode(fs, path);
    if(minixfs_access(fs, path, F_OK) == -1) {
        errno = ENOENT;
    }
    if(!inode) 
    {
        // fprintf(stderr, "In here\n");
        return -1;
    }

    inode->mode = new_permissions | (inode->mode >> RWX_BITS_NUMBER) << RWX_BITS_NUMBER;
    clock_gettime(CLOCK_REALTIME, &inode->ctim);
    return 0;
}

int minixfs_chown(file_system *fs, char *path, uid_t owner, gid_t group) {
    // Land ahoy!
    inode * inode = get_inode(fs, path);
    if(minixfs_access(fs, path, F_OK) == -1) {
        errno = ENOENT;
    }
    if(!inode) {
        return -1;
    }
    // uid_t uid = inode->uid;
    // gid_t gid = inode->gid;
    if(owner != (uid_t)-1) {
        inode->uid = owner;
    }
    if(group != (gid_t)-1) {
        inode->gid = group;
    }
    clock_gettime(CLOCK_REALTIME, &inode->ctim);
    return 0;
}

inode *minixfs_create_inode_for_path(file_system *fs, const char *path) {
    // Land ahoy!
    if(get_inode(fs, path) != NULL) {
        return NULL;
    }

    const char * filename;
    inode * parent = parent_directory(fs, path, &filename);
    if(valid_filename(filename) != 1) {
    //     return NULL;
    }  

    data_block_number num_of_full_blocks = parent->size / sizeof(data_block);
    ssize_t remainder = parent->size / sizeof(data_block);

    if(num_of_full_blocks >= NUM_DIRECT_BLOCKS && parent->indirect == UNASSIGNED_NODE) {
        if(add_single_indirect_block(fs, parent) == -1) {
            return NULL;
        }
    }

    inode_number node_index = first_unused_inode(fs);
    if(node_index == -1) {
        return NULL;
    }

    init_inode(parent, fs->inode_root + node_index);
    data_block *last_block;

    if(remainder == 0) {
        data_block_number new_block_idx;
        if(parent->indirect == UNASSIGNED_NODE) {
            new_block_idx = add_data_block_to_inode(fs, parent);
        } else {
            data_block_number *indirect_blocks = (data_block_number*)(parent->indirect + fs->data_root);
            new_block_idx = add_data_block_to_indirect_block(fs, indirect_blocks);
        }
        if(new_block_idx == -1) {
            return NULL;
        }
        last_block = fs->data_root + new_block_idx;
    } else {
        if(parent->indirect == UNASSIGNED_NODE) {
            last_block = fs->data_root + parent->direct[num_of_full_blocks];
        } else {
           data_block_number *indirect_blocks = (data_block_number*)(parent->indirect + fs->data_root);
           last_block = fs->data_root + indirect_blocks[num_of_full_blocks - NUM_DIRECT_BLOCKS]; 
        }
    }
    char *temp = calloc(1, FILE_NAME_LENGTH);
    strncpy(temp, filename, FILE_NAME_LENGTH);
    minixfs_dirent source = {temp, node_index};
    make_string_from_dirent((char*)last_block, source);
    parent->size += FILE_NAME_LENGTH;
    free(temp);
    return fs->inode_root + node_index;
}

ssize_t minixfs_virtual_read(file_system *fs, const char *path, void *buf,
                             size_t count, off_t *off) {
    if (!strcmp(path, "info")) {
        // TODO implement the "info" virtual file here
        char* map = GET_DATA_MAP(fs->meta);
        ssize_t num_used_block = 0;
        for(uint64_t i = 0; i < fs->meta->dblock_count; i++) {
            if(map[i] == 1) {
                num_used_block++;
            }
        }
        char * s = block_info_string(num_used_block);
        if(*off > (off_t)strlen(s)) {
            return 0;
        }
        count = ( count > strlen(s)-*off ? (strlen(s) - *off) : count );
        memmove(buf, s+*off, count);
        *off+= count;
        return count;
    }

    errno = ENOENT;
    return -1;
}

ssize_t minixfs_write(file_system *fs, const char *path, const void *buf,
                      size_t count, off_t *off) {
    // X marks the spot
    inode *inode = get_inode(fs, path);
    if(inode == NULL) {
        inode = minixfs_create_inode_for_path(fs, path);
        if(inode == NULL) {
            errno = ENOSPC;
            return -1;
        }
    }

    size_t needed_block = (*off + count) / sizeof(data_block);
    if((*off + count) % sizeof(data_block) != 0) {
        needed_block++;
    }
    if(minixfs_min_blockcount(fs, path, (int)needed_block) == -1) {
        errno = ENOSPC;
        return -1;
    }

    size_t block_num = *off / sizeof(data_block);
    size_t block_offset = *off % sizeof(data_block);
    off_t end = *off + count;
    size_t byte_w = 0;

    while(*off < end) {
        data_block curr_block;
        if(block_num >= NUM_DIRECT_BLOCKS) {
            data_block_number *indirect_blocks = (data_block_number*)(inode->indirect + fs->data_root);
            curr_block = fs->data_root[indirect_blocks[block_num - NUM_DIRECT_BLOCKS]]; 
        } else {
            curr_block = fs->data_root[inode->direct[block_num]];
        }
        size_t byte_to_write = sizeof(data_block) - block_offset;
        if((end - *off) < (off_t)byte_to_write) {
            byte_to_write = (end - *off);
        }
        memcpy(curr_block.data, buf + byte_to_write, byte_to_write);
        *off += byte_to_write;
        byte_w += byte_to_write;
        block_offset = 0;
        ++block_num;
    }

    clock_gettime(CLOCK_REALTIME, &inode->atim);
    clock_gettime(CLOCK_REALTIME, &inode->mtim);
    return -1;
}

ssize_t minixfs_read(file_system *fs, const char *path, void *buf, size_t count,
                     off_t *off) {
    const char *virtual_path = is_virtual_path(path);
    if (virtual_path)
        return minixfs_virtual_read(fs, virtual_path, buf, count, off);
    // 'ere be treasure!

    inode * inode = get_inode(fs, path);

    if(!inode) {
        errno = ENOENT;
        return -1;
    }

    if((uint64_t)*off >= inode->size) {
        return 0;
    }

    size_t block_num = *off / sizeof(data_block);
    size_t block_offset = *off % sizeof(data_block);
    off_t end = 0;
    if((*off + count) >= inode->size) {
        end = inode->size;
    } else {
        end = *off + count;
    }

    size_t byte_read = 0;
    while(*off < end) {
        data_block curr_block;
        if(block_num >= NUM_DIRECT_BLOCKS) {
            data_block_number *indirect_blocks = (data_block_number*)(inode->indirect + fs->data_root);
            curr_block = fs->data_root[indirect_blocks[block_num - NUM_DIRECT_BLOCKS]]; 
        } else {
            curr_block = fs->data_root[inode->direct[block_num]];
        }
        size_t byte_to_read = sizeof(data_block) + block_offset;
        if((end - *off) < (off_t)byte_to_read) {
            byte_to_read = (end - *off);
        }
        memcpy(buf + byte_read, curr_block.data, byte_to_read);
        *off += byte_to_read;
        byte_read += byte_to_read;
        block_offset = 0;
        ++block_num;
    }
    clock_gettime(CLOCK_REALTIME, &inode->atim);
    return byte_read;
}
