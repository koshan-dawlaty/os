#include "bdev.h"
#include "lib/list.h"
#include <stdlib.h>
#include <vfs.h>

enum tmpfs_node_type
{
    TMPFS_NODE_DIR,
    TMPFS_NODE_FILE
};

struct tmpfs_node
{
    const char *name;
    struct list2 list;
    union
    {
        struct list2 *dir_head; /* type == TMPFS_NODE_DIR, array */
        void *data /* type == TMPFS_NODE_FILE */;
    };

    enum tmpfs_node_type type;
};

#define TMPFS_ENTRY(node) (struct tmpfs_node *)node->fs_private

static struct tmpfs_node *tmpfs_node_alloc(const char *name,
                                           enum tmpfs_node_type type)
{
    struct tmpfs_node *out = malloc(sizeof(struct tmpfs_node));

    out->name = name;
    out->list = LIST2_INIT();
    out->type = type;

    return out;
}

struct tmpfs_node *tmpfs_node_alloc_file(const char *name,
                                         enum tmpfs_node_type type)
{
    struct tmpfs_node *out = tmpfs_node_alloc(name, type);
    out->data = malloc(0);
    return out;
}

struct tmpfs_node *tmpfs_node_alloc_dir(const char *name,
                                        enum tmpfs_node_type type)
{
    struct tmpfs_node *out = tmpfs_node_alloc(name, type);
    out->dir_head = NULL;
    return out;
}

struct dentry *tmpfs_dentry_append(struct dentry *vfs_parent, char *name,
                                   struct superblock *sb)
{
    struct tmpfs_node *parent = TMPFS_ENTRY(vfs_parent->inode);

    struct tmpfs_node *node = tmpfs_node_alloc_file(name, TMPFS_NODE_FILE);
    struct inode *inode_new = new_inode(0, 0, node, sb);
    struct dentry *entry_new =
        new_dentry(name, vfs_parent, inode_new, DENTRY_FILE);

    list2_add(&node->list, &parent->list);

    return entry_new;
}

/* TODO: a lot of this should not be used with a non ramdisk bev */
struct tmpfs_superblock
{
    struct list2 files;
};

dentry *tmpfs_mount(struct bdev *dev)
{
}

const struct fs_type tmpfs_type = {.name = "tmpfs",
                                   .mount = tmpfs_mount,

                                   .inode_functions = {},
                                   .dentry_functions = {
                                       .append_dentry = tmpfs_dentry_append,
                                   }};
