#include "ext2.h"
#include "vfs.h"
#include <kernel/utils.h>
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
        // printf("%d\n", offset);

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
    free(rootdir);

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
                    free(node);
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

dentry *ext2_dentry_to_dentry(ext2_dentry *dentry, superblock *sb)
{

    return new_dentry(dentry->name, ext2_iget(dentry->inode, sb));
}

void ext2_append_dir(ext2_inode *dir, ext2_inode *node, superblock *sb)
{
    ext2_dentry **entries = ext2_parse_directory(dir, sb);

    size_t i = 0;

    while (entries[i] != NULL)
    {
        i++;
    }
}
