#include "dcache.h"
#include "err.h"
#include "vfs_internal.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs.h>

file *open_file(char *path)
{

    dentry *dentry = lookup_path(path);
    if (IS_ERR(dentry))
    {
        return (void *)dentry;
    }

    file *out = new_file(dentry);

    open_files_len++;
    /* TODO: this will result in open_files fragmenting when files are closed */
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

    current_superblock->type->inode_functions.write_inode(
        node, (uint8_t *)buf, write_size, off, current_superblock);

    add_file_offset(file, write_size);
    return write_size;
}

file *lookup_descriptor(int fd)
{
    return open_files[fd];
}

file *create_file(char *name)
{

    inode *new = current_superblock->type->inode_functions.alloc_inode(
        current_superblock);

    dentry *out = current_superblock->type->dentry_functions.append_dentry(
        get_root_dir(), name, new->number, current_superblock);

    return new_file(out);
}
