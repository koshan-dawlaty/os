#include "vfs_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <vfs.h>
// TODO: bad
#include "../ext2/ext2.h"

superblock *current_superblock = NULL;

file **open_files = NULL;
size_t open_files_len = 0;

void init_vfs()
{
    open_files = malloc(0);

    // TODO: change this
    ext2_fs_type.mount();
    current_superblock = ext2_fs_type.superblock_functions.parse_superblock();
    // printf("superblock:%p\n", current_superblock);
}

superblock *new_superblock(size_t blocksize, void *fs_private,
                           const struct fs_type *type)
{

    superblock *out = malloc(sizeof(superblock));

    out->blocksize = blocksize;
    out->fs_private = fs_private;
    out->type = (struct fs_type *)type;

    return out;
}

dentry *new_dentry(char *name, inode *inode)
{
    dentry *out = malloc(sizeof(dentry));
    out->inode = inode;
    out->name = name;

    return out;
}

file *new_file(dentry *entry)
{
    file *out = malloc(sizeof(file));
    out->entry = entry;
    out->descriptor = 0;
    out->offset = 0;

    return out;
}

inode *new_inode(ino_t number, inode_size size, inode_type type,
                 void *fs_private)
{
    inode *out = malloc(sizeof(inode));

    out->number = number;
    out->size = size;
    out->type = type;

    out->fs_private = fs_private;

    return out;
}

void register_filesystem(struct fs_type *type)
{
}
