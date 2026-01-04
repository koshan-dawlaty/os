#include "dcache.h"
#include "mutex.h"
#include "spinlock.h"
#include "vfs_internal.h"
#include <assert.h>
#include <ata/ata.h>
#include <err.h>
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

struct dentry *rootdir = NULL;

struct dentry *get_root_dir()
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
    rootdir->parent = rootdir;

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
    new_root->parent = root;
    /* TODO: cast discards qualifiers */
    new_root->name = (char *)new_name;
    add_dcache(root, new_root);

    return 0;
}

struct superblock *new_superblock(size_t blocksize, void *fs_private,
                                  struct bdev *dev, const struct fs_type *type)
{
    struct superblock *out = malloc(sizeof(superblock));

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
dentry *dentry_mount_point_alloc(struct superblock *sb, struct inode *root)
{
    struct dentry *out = malloc(sizeof(struct dentry));
    out->type = DENTRY_MOUNT_POINT;
    out->name = ".";
    out->parent = NULL;
    out->inode = root;
    /* might be redundant */
    out->inode->sb = sb;

    return out;
}

void free_dentry(dentry *entry)
{
    free(entry->name);
    /* TODO: we need to do this but it messes up ext2 */
    inode_free(entry->inode);
}

struct file *new_file(struct dentry *entry, int flags)
{
    file *out = malloc(sizeof(file));
    out->entry = entry;
    out->descriptor = 0;
    out->offset = 0;
    out->flags = flags;

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
        UNREACHABLE("dentry must have valid type");
        break;
    }

    return out;
}

void file_free(struct file *file)
{
    /* TODO: drop dentry? */
    /* free(file); */
}

struct inode *new_inode(ino_t number, inode_size size, void *fs_private,
                        struct superblock *sb)
{
    struct inode *out = malloc(sizeof(struct inode));

    out->number = number;
    out->size = size;

    out->fs_private = fs_private;

    out->sb = sb;
    out->lock = MUTEX_INIT();

    return out;
}

void inode_free(struct inode *node)
{
    NOT_NULL(node->sb->type->inode_functions.inode_free,
             "filesystem did not implement function inode_free")
    node->sb->type->inode_functions.inode_free(node);
    free(node);
}

int delete_dentry(struct dentry *entry)
{

    int retval = entry->inode->sb->type->dentry_functions.remove_dentry(
        entry->parent, entry->name, entry->inode->sb);
    remove_dcache(entry);

    return retval;
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

    ASSERT(entry->type == DENTRY_DIR || entry->type == DENTRY_MOUNT_POINT,
           "attempt to read directory on non directory dentry");

    struct superblock *sb = entry->inode->sb;
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
