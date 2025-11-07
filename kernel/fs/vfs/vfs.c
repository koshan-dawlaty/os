#include "ata/ata.h"
#include "dcache.h"
#include "vfs_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <vfs.h>
// TODO: bad
#include "../ext2/ext2.h"

superblock *current_superblock = NULL;

file **open_files = NULL;
size_t open_files_len = 0;

dentry *get_root_dir()
{
    inode *node = current_superblock->type->inode_functions.get_inode(
        2, current_superblock);

    /* TODO: maybe dont make a new dentry? */
    dentry *entry = new_dentry("/", NULL, node, DENTRY_DIR);

    return entry;
}

void init_vfs()
{
    open_files = malloc(0);

    struct bdev *dev = get_ata_drive(0);

    // TODO: change this
    ext2_fs_type.mount(dev);
    current_superblock =
        ext2_fs_type.superblock_functions.parse_superblock(dev);

    init_dcache();
    // printf("superblock:%p\n", current_superblock);
}

superblock *new_superblock(size_t blocksize, void *fs_private, struct bdev *dev,
                           const struct fs_type *type)
{

    superblock *out = malloc(sizeof(superblock));

    out->blocksize = blocksize;
    out->fs_private = fs_private;
    out->type = (struct fs_type *)type;
    out->dev = dev;

    return out;
}

dentry *new_dentry(char *name, dentry *parent, inode *inode, dentry_type type)
{
    dentry *out = malloc(sizeof(dentry));
    out->inode = inode;
    out->name = name;
    out->parent = parent;
    out->type = type;

    return out;
}
void free_dentry(dentry *entry)
{
    free(entry->name);
    /* TODO: free inode */
}

file *new_file(dentry *entry)
{
    file *out = malloc(sizeof(file));
    out->entry = entry;
    out->descriptor = 0;
    out->offset = 0;

    return out;
}

inode *new_inode(ino_t number, inode_size size, void *fs_private)
{
    inode *out = malloc(sizeof(inode));

    out->number = number;
    out->size = size;

    out->fs_private = fs_private;

    return out;
}

int delete_dentry(dentry *entry)
{

    remove_dcache(entry);

    return current_superblock->type->dentry_functions.remove_dentry(
        entry->parent, entry->name, current_superblock);
}

void register_filesystem(struct fs_type *type)
{
}
