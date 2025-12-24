#include "dcache.h"
#include "err.h"
#include "panic.h"
#include "vfs_internal.h"
#include <ata/ata.h>
#include <fcntl.h>
#include <scheduler.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vfs.h>
/* TODO: bad */
#include "../ext2/ext2.h"
#include "../tmpfs/tmpfs.h"

struct fdlist *basic_open_files = NULL;

dentry *rootdir = NULL;

dentry *get_root_dir()
{
    return rootdir;
}

void init_vfs()
{
    basic_open_files = new_fdlist();

    struct bdev *dev = get_ata_drive(0);

    /* struct fs_type type = tmpfs_type; */
    struct fs_type type = ext2_fs_type;
    rootdir = type.mount(dev);

    /* rootdir->inode->sb->type->dentry_functions.append_dentry( */
    /*     rootdir, "abc", rootdir->inode->sb); */

    /* rootdir = new_dentry(".", NULL, node, DENTRY_DIR); */

    init_dcache();

    vfs_mount(rootdir, "tmpfs", tmpfs_type, NULL);
}

int vfs_mount(struct dentry *root, const char *new_name, struct fs_type fs,
              struct bdev *dev)
{
    struct dentry *new_root = fs.mount(dev);
    /* TODO: cast discards qualifiers */
    new_root->name = (char *)new_name;

    add_dcache(root, new_root);
    return 0;
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

    switch (entry->type)
    {
    case DENTRY_DEVICE:
    case DENTRY_FILE:
        break;
    case DENTRY_MOUNT_POINT:
    case DENTRY_DIR:
        out->dir_context = DIR_CONTEXT_INIT();
        break;
    case DENTRY_INVALID:
        break;
    }

    return out;
}

inode *new_inode(ino_t number, inode_size size, void *fs_private,
                 superblock *sb)
{
    inode *out = malloc(sizeof(inode));

    out->number = number;
    out->size = size;

    out->fs_private = fs_private;

    out->sb = sb;

    return out;
}

int delete_dentry(dentry *entry)
{
    remove_dcache(entry);

    return entry->inode->sb->type->dentry_functions.remove_dentry(
        entry->parent, entry->name, entry->inode->sb);
}

void register_filesystem(struct fs_type *type)
{
}

/*
 * TODO: this currently will not use dcache for actual file system entries
 * The issue with this is that both dcache and the filesystem have duplicate
 * information
 */
struct dentry *dir_readdir(dentry *entry, struct dir_context *context)
{
    superblock *sb = entry->inode->sb;

    struct dentry *dcache = dcache_readdir(entry, context);

    /* printf("%p\n", dcache); */

    if (!IS_ERR(dcache) && dcache != NULL)
    {
        /* printf("got %s from dcache\n", dcache->name); */
        context->num_read++;
        return dcache;
    }

    struct dentry *out =
        sb->type->dentry_functions.read_dir(entry, context, sb);
    context->num_read++;
    if (out != NULL)
    {
        /* printf("got %s from not dcache\n", out->name); */
    }

    return out;
}
