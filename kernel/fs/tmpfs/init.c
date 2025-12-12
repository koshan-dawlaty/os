#include "bdev.h"
#include "lib/list.h"
#include <stdlib.h>
#include <vfs.h>

enum tmpfs_dentry_type
{
    TMPFS_DENTRY_DIR,
    TMPFS_DENTRY_FILE
};

struct tmpfs_dentry
{
    const char *name;
    struct list2 list;
    struct tmpfs_dentry *parent;

    union
    {
        struct list2 *dir_head; /* type == TMPFS_DENTRY_DIR, array */
        void *data /* type == TMPFS_DENTRY_FILE */;
    };

    enum tmpfs_dentry_type type;
};

static struct tmpfs_dentry *tmpfs_dentry_alloc(const char *name,
                                               struct tmpfs_dentry *parent,
                                               enum tmpfs_dentry_type type)
{
    struct tmpfs_dentry *out = malloc(sizeof(struct tmpfs_dentry));

    out->name = name;
    out->parent = parent;
    out->parent = parent;
    out->type = type;

    return out;
}

struct tmpfs_dentry *tmpfs_dentry_alloc_file(const char *name,
                                             struct tmpfs_dentry *parent,
                                             enum tmpfs_dentry_type type)
{
    struct tmpfs_dentry *out = tmpfs_dentry_alloc(name, parent, type);
    out->data = malloc(0);
    return out;
}

struct tmpfs_dentry *tmpfs_dentry_alloc_dir(const char *name,
                                            struct tmpfs_dentry *parent,
                                            enum tmpfs_dentry_type type)
{
    struct tmpfs_dentry *out = tmpfs_dentry_alloc(name, parent, type);
    out->dir_head = NULL;
    return out;
}

/* TODO: a lot of this should not be used with a non ramdisk bev */
struct tmpfs_superblock
{
    struct list2 files;
};

dentry *tmpfs_mount(struct bdev *dev)
{
}

const struct fs_type tmpfs_type = {.name = "tmpfs", .mount = tmpfs_mount};
