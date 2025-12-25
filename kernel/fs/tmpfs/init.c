/* TODO: a lot of this should not be used with a non ramdisk bev */
#include "tmpfs.h"
#include <assert.h>
#include <bdev.h>
#include <container_of.h>
#include <err.h>
#include <errno.h>
#include <lib/list.h>
#include <stdlib.h>
#include <string.h>
#include <vfs.h>

struct tmpfs_superblock
{
    struct list2 files;
    ino_t inode_count;
};

#define TMPFS_SB(s) ((struct tmpfs_superblock *)s->fs_private)

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
        struct
        {
            void *data;
            size_t len;
        } file; /* type == TMPFS_NODE_FILE */
    };

    enum tmpfs_node_type type;
};

#define TMPFS_NODE(node) (struct tmpfs_node *)node->fs_private

static struct tmpfs_superblock *tmpfs_super_alloc()
{
    struct tmpfs_superblock *out = malloc(sizeof(struct tmpfs_superblock));

    out->inode_count = 0;

    return out;
}

static struct tmpfs_node *tmpfs_node_alloc(const char *name,
                                           enum tmpfs_node_type type,
                                           struct superblock *sb)
{
    struct tmpfs_node *out = malloc(sizeof(struct tmpfs_node));

    out->name = name;
    out->list = LIST2_INIT();
    out->type = type;

    return out;
}

struct tmpfs_node *tmpfs_node_alloc_file(const char *name,
                                         enum tmpfs_node_type type,
                                         struct superblock *sb)
{
    struct tmpfs_node *out = tmpfs_node_alloc(name, type, sb);
    out->file.data = malloc(0);
    out->file.len = 0;
    return out;
}

struct tmpfs_node *tmpfs_node_alloc_dir(const char *name,
                                        enum tmpfs_node_type type,
                                        struct superblock *sb)
{
    struct tmpfs_node *out = tmpfs_node_alloc(name, type, sb);
    out->dir_head = &out->list;
    return out;
}

struct dentry *tmpfs_dentry_append(struct dentry *vfs_parent, char *name,
                                   struct superblock *sb)
{
    struct tmpfs_node *parent = TMPFS_NODE(vfs_parent->inode);

    ASSERT(parent->type == TMPFS_NODE_DIR, "entry needs to be a dir");

    struct tmpfs_node *node = tmpfs_node_alloc_file(name, TMPFS_NODE_FILE, sb);
    struct inode *inode_new =
        new_inode(TMPFS_SB(sb)->inode_count++, 0, node, sb);
    TMPFS_SB(sb)->inode_count++;
    struct dentry *entry_new =
        new_dentry(name, vfs_parent, inode_new, DENTRY_FILE);

    /* if (parent->dir_head == NULL) */
    /* { */
    /*     parent->dir_head = &node->list; */
    /* } */
    /* else */
    /* { */
    list2_add(&node->list, parent->dir_head);
    /* } */

    return entry_new;
}

struct dentry *tmpfs_read_dir(struct dentry *vfs_dir,
                              struct dir_context *context,
                              struct superblock *sb)
{
    struct tmpfs_node *dir = TMPFS_NODE(vfs_dir->inode);
    /* TODO: this will iterate through the list every time instead of indexing
     * it like an array, maybe make dir_context store a pointer, or something
     * similar
     * also context->fs_offset == context->num_read, might be able to not
     * increment (this is probably a really bad idea and will be bad later */

    /* printf("number:%p\n", context); */

    size_t i = 0;
    struct list2 *pos;
    list2_for_each(pos, &dir->list)
    {
        if (i == context->fs_offset)
        {
            context->fs_offset++;
            goto found_entry;
        }

        i++;
    }

    if (context->fs_offset == i)
    {
        return NULL;
    }
    else
    {
        return ERR_PTR(-EOVERFLOW);
    }

found_entry:;
    struct tmpfs_node *node = LIST_ENTRY(pos, struct tmpfs_node, list);

    /* printf("tmpfs:%s\n", node->name); */

    struct inode *out_node =
        new_inode(TMPFS_SB(sb)->inode_count, node->file.len, node, sb);
    TMPFS_SB(sb)->inode_count++;

    struct dentry *out =
        new_dentry(/* TODO: cast discards qualifiers */ (char *)node->name,
                   vfs_dir, out_node, DENTRY_FILE);

    return out;
}

void tmpfs_read_inode(struct inode *vfs, u8 *buf, size_t n, size_t offset)
{
    struct tmpfs_node *node = TMPFS_NODE(vfs);

    ASSERT(node->type == TMPFS_NODE_FILE, "node should be a file");

    if (node->file.len <= n + offset)
    {
        n = node->file.len;
    }
    memcpy(buf, node->file.data + offset, n);
}

void tmpfs_write_inode(struct inode *vfs, const u8 *buf, size_t n,
                       size_t offset)
{
    struct tmpfs_node *node = TMPFS_NODE(vfs);

    if (n + offset >= node->file.len)
    {
        node->file.data = realloc(node->file.data, n + offset);
        node->file.len = n + offset;
        vfs->size = n + offset;
    }
    memcpy(node->file.data + offset, buf, n);
}

struct superblock *tmpfs_parse_superblock(struct bdev *dev)
{
    struct tmpfs_superblock *tmpfs = tmpfs_super_alloc();
    struct superblock *out = new_superblock(-1, tmpfs, dev, &tmpfs_type);

    return out;
}

dentry *tmpfs_mount(struct bdev *dev)
{
    /* assume that the caller wants to create a new filesystem */
    struct superblock *sb = tmpfs_parse_superblock(dev);
    struct tmpfs_node *node = tmpfs_node_alloc_dir(".", TMPFS_NODE_DIR, sb);
    struct inode *root = new_inode(TMPFS_SB(sb)->inode_count, 0, node, sb);
    TMPFS_SB(sb)->inode_count++;

    struct dentry *out = dentry_mount_point_alloc(sb, root);
    return out;
}

int tmpfs_dentry_remove(dentry *parent, char *name, superblock *sb)
{
    struct tmpfs_node *node = TMPFS_NODE(parent->inode);
    ASSERT(node->type == TMPFS_NODE_DIR,
           "tmpfs_remove_dentry needs a directory passed as the dentry");

    list2 *pos;
    list_for_each(pos, node->dir_head)
    {
        struct tmpfs_node *current = LIST_ENTRY(pos, struct tmpfs_node, list);

        if (!strcmp(current->name, name))
        {
            list2_remove(pos, &node->dir_head);
            /* TODO: bad free */
            free(current);
            return 0;
        }
    }

    return -ENOENT;
}

const struct fs_type tmpfs_type = {

    .name = "tmpfs",
    .mount = tmpfs_mount,

    .inode_functions =
        {
            .read_inode = tmpfs_read_inode,
            .write_inode = tmpfs_write_inode,
        },
    .dentry_functions =
        {
            .append_dentry = tmpfs_dentry_append,
            .remove_dentry = tmpfs_dentry_remove,
            .read_dir = tmpfs_read_dir,
        },

    .superblock_functions =
        {
            .parse_superblock = tmpfs_parse_superblock,
        },
};
