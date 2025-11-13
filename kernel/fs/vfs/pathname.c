#include "alloca.h"
#include "complex.h"
#include "dcache.h"
#include "err.h"
#include "kernel/types.h"
#include "scheduler.h"
#include "vfs.h"
#include "vfs_internal.h"
#include <errno.h>
#include <panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

dentry **pathname_get_dentries(dentry *dir)
{
    return current_superblock->type->dentry_functions.get_dentries(
        dir, current_superblock);
}

inode *pathname_get_inode(ino_t number)
{
    return current_superblock->type->inode_functions.get_inode(
        number, current_superblock);
}

inode *pathname_get_root_dir()
{
    /* TODO: will break if root inode is not 2 */

    return pathname_get_inode(2);
}

dentry *parse_dentries_component(dentry **dentries, char *component)
{
    size_t i = 0;
    while (dentries[i] != NULL)
    {

        if (!strcmp(dentries[i]->name, component))
        {
            return dentries[i];
        }

        i++;
    }
    return NULL;
}

size_t parse_component(const char *path, inode **current_node,
                       dentry **current_dir, struct pathname_lookup_flags flags)
{
    /* TODO: stupid */
    bool starts_with_slash = false;

    if (*path == '/')
    {
        starts_with_slash = true;
        path++;
    }

    size_t component_len = 0;
    while (path[component_len] != 0 && path[component_len] != '/')
    {
        component_len++;
    }

    bool final_component = false;
    if (path[component_len] == 0)
    {
        final_component = true;
    }

    char *component = malloc(component_len + 1);
    memcpy(component, path, component_len);
    component[component_len] = 0;

    dentry *entry = get_dcache(component, *current_dir);

    if (entry == NULL)
    {
        dentry **dentries = pathname_get_dentries(*current_dir);

        entry = parse_dentries_component(dentries, component);

        if (entry == NULL)
        {

            if (flags.create && final_component)
            {

                entry =
                    (*current_dir)
                        ->inode->sb->type->dentry_functions.append_dentry(
                            *current_dir, component, (*current_dir)->inode->sb);
            }
            else
            {
                goto not_found;
            }
        }
        else if (flags.create)
        {
            goto already_exists;
        }

        add_dcache(*current_dir, entry);
    }
    else if (flags.create && final_component)
    {
        goto already_exists;
    }

    *current_dir = entry;

    if (entry->type == DENTRY_DIR)
    {
        *current_node = entry->inode;
    }
    else if (entry->type == DENTRY_FILE)
    {
        if (!final_component)
        {
            goto not_dir;
        }
        *current_dir = entry;
    }
    else if (entry->type == DENTRY_DEVICE)
    {
        if (!final_component)
        {
            goto not_found;
        }

        *current_dir = entry;
    }
    else
    {
        panic("unhandled dentry type in lookup_path!!!");
    }

    /* *current_dir = ERR_PTR(-ENOENT); */

out:

    /* TODO stupid */
    if (starts_with_slash)
    {
        component_len++;
    }
    return component_len;

not_found:
    *current_dir = ERR_PTR(-ENOENT);
    goto out;
already_exists:
    *current_dir = ERR_PTR(-EEXIST);
    goto out;
not_dir:
    *current_dir = ERR_PTR(-ENOTDIR);
    goto out;
}

dentry *lookup_path(const char *path, struct pathname_lookup_flags flags)
{
    inode *current_node = NULL;
    dentry *current_dir = NULL;
    size_t component_offset = 0;

    if (*path != '/')
    {

        struct task *current_task = get_current_task();
        if (current_task != NULL)
        {
            current_dir = current_task->working_dir;
        }
        else
        {
            current_dir = NULL;
        }

        if (current_dir == NULL)
        {
            current_node = pathname_get_root_dir();
            current_dir = get_root_dir();
        }
        else
        {
            current_node = current_dir->inode;
        }
    }
    else
    {
        component_offset++;
        current_node = pathname_get_root_dir();
        current_dir = get_root_dir();
    }

    while (*(path + component_offset) != 0)
    {

        component_offset += parse_component(path + component_offset,
                                            &current_node, &current_dir, flags);

        /* component_offset++; */

        if (IS_ERR(current_dir))
        {
            return current_dir;
        }
    }

    return current_dir;
}
