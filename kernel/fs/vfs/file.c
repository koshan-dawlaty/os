#include "vfs_internal.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs.h>

file *open_file(char *path)
{

    // printf("len %d\n", open_files_len);
    // printf("pointer: %p\n", current_superblock);
    dentry *dentry = current_superblock->type->dentry_functions.find_dentry(
        path, current_superblock);
    // printf("dentry:%p\n", dentry);

    file *out = new_file(dentry);

    open_files_len++;
    // TODO: this will result in open_files fragmenting
    open_files = realloc(open_files, open_files_len * sizeof(file *));

    open_files[open_files_len - 1] = out;

    out->descriptor = open_files_len - 1;

    return out;
}

int read_file(file *file, char *buf, size_t n, size_t off)
{

    inode *node = file->entry->inode;

    size_t read_size = n;

    if (off + n > node->size - off)
    {
        read_size = node->size - off;
    }

    current_superblock->type->inode_functions.read_inode(
        node, (uint8_t *)buf, read_size, off, current_superblock);

    add_file_offset(file, read_size);

    return read_size;
}

file *lookup_descriptor(int fd)
{
    return open_files[fd];
}
