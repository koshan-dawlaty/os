#include "err.h"
#include "fcntl.h"
#include "kernel/paging.h"
#include "panic.h"
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
    dentry *dentry = lookup_path(path);
    if (IS_ERR(dentry))
    {
        /* printf("error:%d\n", GET_ERR(dentry)); */
        return (void *)dentry;
    }

    file *out = new_file(dentry);

    open_files_len++;
    // TODO: this will result in open_files fragmenting
    open_files = realloc(open_files, (open_files_len) * sizeof(file *));

    /* printf("open_files:%p\n", open_files); */

    open_files[open_files_len - 1] = out;
    out->descriptor = open_files_len - 1;
    /* printf("number:%d, name:%s\n", out->descriptor, out->entry->name); */

    return out;
}

int read_file(file *file, char *buf, size_t n, size_t off)
{

    inode *node = file->entry->inode;

    size_t read_size = n;
    /* printf("size:%d\n", node->size); */

    if (off + n > node->size)
    {
        read_size = node->size - off;
    }

    current_superblock->type->inode_functions.read_inode(
        node, (uint8_t *)buf, read_size, off, current_superblock);

    /* if (off > 2000) */
    /* { */
    /*     printf("buf:%s\n", buf); */
    /* } */

    add_file_offset(file, read_size);
    /* printf("returning %d\n", read_size); */

    return read_size;
}

int write_file(file *file, char *buf, size_t n, size_t off)
{

    inode *node = file->entry->inode;

    size_t write_size = n;
    printf("size:%p\n", write_size);
    /* if (off + n > node->size) */
    /* { */
    /*     write_size = node->size - off; */
    /* } */

    current_superblock->type->inode_functions.write_inode(
        node, (uint8_t *)buf, write_size, off, current_superblock);

    add_file_offset(file, write_size);
    return write_size;
}

file *lookup_descriptor(int fd)
{
    return open_files[fd];
}
