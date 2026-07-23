#include "container_of.h"
#include "ext2.h"
#include "lib/list.h"
#include <err_ptr.h>
#include <errno.h>
#include <kernel/utils.h>
#include <lib/log.h>
#include <panic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>
#include <vfs.h>

struct dentry *ext2_readdir(struct dentry *entry, struct dir_context *context,
                            superblock *sb)
{
read_entry:;

    if (context->fs_offset == EXT2_BLOCK_SIZE(sb))
    {
        return NULL;
    }

    ext2_inode *node = EXT2_INODE(entry->inode);

    size_t size = ext2_get_inode_size(node);
    u8 *buf = malloc(size);
    u8 *buf_ptr = buf;
    ext2_read_inode(node, buf, size, 0, sb);
    if (context->fs_offset > EXT2_BLOCK_SIZE(sb))
    {
        return ERR_PTR(-EOVERFLOW);
    }

    buf += context->fs_offset;

    size_t node_index = *(u32 *)(buf);

    u16 name_len = buf[6];
    /* TODO: there may be some fields that could be used to support longer
     * filenames */

    char *name_ptr = (char *)buf + 8;
    u16 total_size = *(u16 *)(buf + 4);
    if (total_size != 0)
    {
        logd("size of:%d", total_size);
    }

    /* TODO: slightly more efficent way of doing this by checking only the first
     * two characters (dcache does this in dcache_handle_dot) */
    if (strcmp(name_ptr, "..") == 0 || strcmp(name_ptr, ".") == 0)
    {
        context->fs_offset += total_size;
        goto read_entry;
    }

    char *new_name = malloc(name_len + 1);
    memcpy(new_name, name_ptr, name_len);
    /* printf("printing name\n"); */
    new_name[name_len] = 0;

    ext2_inode *current_node = ext2_get_inode(node_index, sb);
    if (IS_ERR(current_node))
    {
        /* error */
        return (void *)current_node;
    }

    struct dentry *out = new_dentry(
        new_name, entry, ext2_get_vfs_node(node_index, current_node, sb),
        ext2_get_dentry_type(current_node));

    context->fs_offset += total_size;
    free(buf_ptr);

    return out;
}

int ext2_remove_dentry(dentry *entry, char *name, superblock *sb)
{

    inode *node = entry->inode;

    /* TODO: this will not work if the size goes beyond one block */
    ext2_inode *ext2_entry = EXT2_INODE(node);

    size_t block_size = EXT2_BLOCK_SIZE(sb);
    u8 *buf = malloc(block_size);
    ext2_read_inode(ext2_entry, buf, EXT2_BLOCK_SIZE(sb), 0, sb);
    size_t i = 0;
    size_t prev = 0;
    bool is_last_entry = false;
    while (i < EXT2_BLOCK_SIZE(sb))
    {
        u8 *raw_entry = buf + i;

        u16 len = *(u16 *)(raw_entry + 4);
        u8 name_len = *(u8 *)(raw_entry + 6);

        if (!strncmp((char *)(raw_entry + 8), name, name_len))
        {
            if (i + len == EXT2_BLOCK_SIZE(sb))
            {
                is_last_entry = true;
            }
            /* i += len; */
            break;
        }

        if (i + len >= EXT2_BLOCK_SIZE(sb))
        {
            return -ENOENT;
        }

        prev = i;
        i += len;
    }

    u8 *to_remove = buf + i;
    /* u16 old_len = align_long(*(u8 *)(to_remove + 6) + 8, 4); */
    u16 old_len = *(u16 *)(to_remove + 4);
    size_t j = i + old_len;

    if (is_last_entry)
    {
        u8 *last = buf + prev;
        *(u16 *)(last + 4) = EXT2_BLOCK_SIZE(sb) - prev;
        goto done;
    }

    size_t move_size = EXT2_BLOCK_SIZE(sb) - j;

    char *copy_buf = malloc(move_size);
    memcpy(copy_buf, buf + j, move_size);
    /* move entries back */
    memcpy(buf + i, copy_buf, move_size);

    /* modify final dentry to point to the end of the block */
    while (i < EXT2_BLOCK_SIZE(sb))
    {

        u8 *raw_entry = buf + i;
        u16 len = *(u16 *)(raw_entry + 4);

        if (i + len + old_len == EXT2_BLOCK_SIZE(sb))
        {

            *(u16 *)(raw_entry + 4) = EXT2_BLOCK_SIZE(sb) - i;
            break;
        }

        i += len;
    }

done:

    ext2_write_inode(node, buf, EXT2_BLOCK_SIZE(sb), 0, sb);

    free(buf);

    return 0;
}

/* this could also be done without a linked list, by appending to a string, but
 * it would be slower as it would have to shift the string every new pathname,
 * although it would use less memory */
struct dentry_pathname_list
{
    struct dentry *entry;
    struct list list;
};
size_t dentry_get_pathname(struct dentry *entry, char *buf, size_t len)
{
    LIST_DEFINE_HEAD(head);

    struct dentry *current = entry;
    struct dentry *prev = NULL;

    /* FIXME: maybe there should be a better way of checking if something is the
     * root directory? */
    while (current != prev)
    {
        struct dentry_pathname_list *l =
            malloc(sizeof(struct dentry_pathname_list));
        l->list = LIST_INIT();
        l->entry = current;
        list_push(&l->list, &head);

        prev = current;
        current = current->parent;
    }

    bool should_include_slash = false;
    size_t i = 0;
    struct list *pos;
    list_for_each(pos, &head)
    {
        struct dentry_pathname_list *l =
            containerof(pos, struct dentry_pathname_list, list);
        const char *name = l->entry->name;

        size_t copy_len = strlen(name);
        if (i + strlen(name) >= len)
        {
            memcpy(buf + i, name, len - i);
            break;
        }

        /* not the first entry being read */
        if (i != 0)
        {
            if (should_include_slash)
            {
                buf[i] = '/';
                i++;
            }
            else
            {
                should_include_slash = true;
            }
        }
        memcpy(buf + i, name, strlen(name));
        i += strlen(name);
    }
    buf[i] = 0;
    return i;
}
