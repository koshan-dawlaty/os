#include "ctype.h"
#include "ext2.h"
#include "kernel/paging.h"
#include "kernel/types.h"
#include "vfs.h"
#include <errno.h>
#include <kernel/utils.h>
#include <panic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_dentries(ext2_dentry **entries)
{
    while (*entries != NULL)
    {

        free(*entries);

        entries++;
    }

    // free(entries);
}

dentry **ext2_get_dentries(dentry *entry, superblock *sb)
{
    ext2_inode *node = EXT2_INODE(entry->inode);

    dentry **out = malloc(sizeof(dentry));
    size_t out_len = 0;

    size_t size = ext2_get_inode_size(node);
    uint8_t *buf = malloc(size);
    ext2_read_inode(node, buf, size, 0, sb);

    size_t i = 0;
    while (i < size)
    {

        uint8_t *raw_dentry = buf + i;

        /* for (size_t j = 0; j < 10; j++) */
        /* { */
        /*     printf("%p ", raw_dentry[j - 5]); */
        /* } */

        uint32_t node_num = *(uint32_t *)(raw_dentry);
        /* printf("inode number:%d\n", node_num); */
        uint16_t name_len = raw_dentry[6];
        /* TODO: there may be some fields that could be used to support longer
         * filenames */

        uint8_t *name_ptr = raw_dentry + 8;

        char *new_name = malloc(name_len + 1);
        memcpy(new_name, name_ptr, name_len);
        /* printf("printing name\n"); */
        new_name[name_len] = 0;
        /* printf("%s\n", new_name); */

        ext2_inode *new_node = ext2_get_inode(node_num, sb);
        inode *vfs_node = ext2_get_vfs_node(node_num, new_node);

        dentry *vfs =
            new_dentry(new_name, entry, ext2_get_vfs_node(node_num, new_node));

        out_len++;
        out = realloc(out, out_len * sizeof(dentry));
        out[out_len - 1] = vfs;

        uint16_t total_size = bytes_to_16_bit_int(raw_dentry[4], raw_dentry[5]);
        /* printf("inode:%d, total size:%d, name:%s\n", node_num, total_size, */
        /*        new_name); */
        /* printf("%d\n", i); */

        i += total_size;
    }

    out_len++;
    out = realloc(out, out_len * sizeof(dentry));
    out[out_len - 1] = NULL;

    return out;
}

/*
 * this returns a heap pointer to heap pointers
 */
ext2_dentry **ext2_parse_directory(ext2_inode *node, superblock *sb)
{

    // printf("find called\n");

    // MAY BREAK
    if ((node->type >> 12) != 4)
    {
        printf("not a directory\n");
        // not a directory
        return NULL;
    }

    uint64_t size = ext2_get_inode_size(node);

    uint8_t *content = malloc(size);
    // size may be wrong
    ext2_read_inode(node, content, size, 0, sb);

    for (size_t i = 0; i < 512; i++)
    {
        // printf("%p ", content[i]);
    }

    ext2_dentry **out = malloc(sizeof(ext2_dentry *));
    size_t out_len = 1;

    int dentry_type_present = EXT2_FEATURE_FLAGS_REQUIRED(sb) & 2;

    uint offset = 0;
    uint name_len = 0;
    while (offset < size)
    {
        uint8_t *ptr = (uint8_t *)(content + offset);
        // MAY BE WRONG
        ext2_inode_index inode =
            bytes_to_32_bit_int(ptr[0], ptr[1], ptr[2], ptr[3]);

        // hopefully the compiler will optimize this into 2 loops
        if (dentry_type_present)
        {
            name_len = (*(ptr + 6));
        }
        else
        {
            // MAY BE WRONG
            name_len = (*(ptr + 6)) | ((*(ptr + 7)) << 8);
        }
        // printf("%d ", name_len);

        char *name = malloc((name_len + 1) * sizeof(char));

        for (uint i = 0; i < name_len; i++)
        {
            // printf("%c", *(ptr + 8 + i));
            name[i] = (*(ptr + 8 + i));
        }
        name[name_len] = '\0';
        // printf("%s\n", name);
        offset += (*(ptr + 4)) | (*(ptr + 5) << 8);
        /* printf("%d\n", offset); */

        ext2_dentry *entry = malloc(sizeof(ext2_dentry));
        entry->inode = inode;
        entry->name = name;

        out[out_len - 1] = entry;

        out_len++;
        out = realloc(out, out_len * sizeof(ext2_dentry));
    }

    out[out_len] = NULL;

    return out;
}

ext2_dentry *ext2_find_dentry(char *path2, superblock *sb)
{

    /* probably useless and can probably be avoided with memcmp */
    char *path = malloc(strlen(path2) + 1);
    memcpy(path, path2, strlen(path2) + 1);

    // printf("%p\n", sb->blocksize);

    // printf("find dentry\n");

    // this is slow, and get inode shouldn't return a heap pointer
    ext2_inode *rootdir = ext2_get_inode(2, sb);
    ext2_dentry **entries = ext2_parse_directory(rootdir, sb);
    /* free(rootdir); */

    uint current_dir_start = 0;
    uint i = 0;

    // printf("this: %s\n", path);
    while (path[i] != '\0')
    {
        /* printf("i is:%d\n", i); */
        if (path[i] == '/')
        {

            uint until_next_slash = i + 1;
            while (path[until_next_slash] != '\0' &&
                   path[until_next_slash] != '/')
            {
                until_next_slash++;
            }
            // printf("aaa%d\n", until_next_slash);

            uint8_t is_last_thing_in_path = path[until_next_slash] == '\0';

            // printf("until_next_slash: %d\n", until_next_slash);

            // TODO: modifying path could be avoided if memcmp was used
            path[until_next_slash] = '\0';

            // printf("path now:%s\n", path);

            uint j = 0;
            while (entries[j] != NULL)
            {
                if (entries[j]->inode == 11)
                {
                    j++;
                    continue;
                }
                // printf("j: %d\n", j);

                // printf("path:%s\n", path + i + 1);
                // printf("name:%s\n", entries[j]->name);
                // printf("result:%p\n", strcmp(path + i + 1,
                // entries[j]->name));

                // may go out of bounds
                if (!strcmp(path + i + 1, entries[j]->name))
                {
                    // printf("path is:%s\n", path);
                    // printf("checking: %s\n", path + until_next_slash);
                    // printf("got: %d\n", *(path + until_next_slash) == '\0');
                    // may go out of bounds
                    if (is_last_thing_in_path)
                    {
                        // printf("exit\n");
                        /* printf("name:%s\n", entries[j]->name); */

                        ext2_dentry *temp = entries[j];
                        ext2_dentry *out = malloc(sizeof(ext2_dentry));
                        memcpy(out, temp, sizeof(ext2_dentry));

                        free_dentries(entries);
                        /* printf("name %s\n", out->name); */
                        /* printf("inode %d\n", out->inode); */

                        return out;
                    }

                    // may break
                    ext2_inode *node = ext2_get_inode(entries[j]->inode, sb);
                    free_dentries(entries);
                    entries = ext2_parse_directory(node, sb);
                    /* free(node); */
                    goto end;
                }

                j++;
            }

        end:
            path[until_next_slash] = '/';
        }

        i++;
    }

    printf("not found\n");
    return NULL;
}

int ext2_remove_dentry(inode *node, char *name, superblock *sb)
{

    /* TODO: this will not work if the size goes beyond one block */
    ext2_inode *ext2_entry = EXT2_INODE(node);

    u8 *buf = malloc(EXT2_BLOCK_SIZE(sb));
    ext2_read_inode(ext2_entry, buf, EXT2_BLOCK_SIZE(sb), 0, sb);
    size_t i = 0;
    while (i < EXT2_BLOCK_SIZE(sb))
    {
        u8 *raw_entry = buf + i;

        u16 len = *(u16 *)(raw_entry + 4);
        u8 name_len = *(u8 *)(raw_entry + 6);

        if (!strncmp((char *)(raw_entry + 8), name, name_len))
        {
            /* i += len; */
            break;
        }

        if (i + len >= EXT2_BLOCK_SIZE(sb))
        {
            return ENOENT;
        }

        i += len;
    }

    u8 *to_remove = buf + i;
    u16 old_len = *(u16 *)(to_remove + 4);
    size_t j = i + old_len;

    /* move entries back */
    memcpy(buf + i, buf + j, j);

    /* modify final dentry to point to the end of the block */
    while (i < EXT2_BLOCK_SIZE(sb))
    {

        u8 *raw_entry = buf + i;
        u16 len = *(u16 *)(raw_entry + 4);

        if (i + len >= EXT2_BLOCK_SIZE(sb) - old_len)
        {

            *(u16 *)(raw_entry + 4) = EXT2_BLOCK_SIZE(sb) - i;
            break;
        }

        i += len;
    }

    ext2_write_inode(node, buf, EXT2_BLOCK_SIZE(sb), 0, sb);

    free(buf);

    return 0;
}
