#include "assert.h"
#include "dcache.h"
#include "vfs.h"
#include "vfs_internal.h"
#include <err.h>

#include <errno.h>
#include <kernel/types.h>
#include <panic.h>
#include <scheduler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static dentry *parse_dentries_component(dentry *dir, const char *component)
{

    struct dir_context context = DIR_CONTEXT_INIT();
    struct dentry *ent = dir_readdir(dir, &context);

    while (ent != NULL && !IS_ERR(ent))
    {

        if (!strcmp(ent->name, component))
        {
            return ent;
        }

        ent = dir_readdir(dir, &context);
    }

    return NULL;
}

static size_t parse_component(const char *path, dentry **current_dir,
                              struct pathname_lookup_flags flags)
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

    /* TODO: maybe dont malloc?, this gets passed to strdup() multiple times */
    char *component = malloc(component_len + 1);
    memcpy(component, path, component_len);
    component[component_len] = 0;

    struct dentry *entry = get_dcache(component, *current_dir);

    if (entry == NULL)
    {

        entry = parse_dentries_component(*current_dir, component);

        if (entry == NULL)
        {

            if (flags.create && final_component)
            {

                if (DENTRY_TYPE_HAS_INODE(flags.type))
                {
                    entry =
                        (*current_dir)
                            ->inode->sb->type->dentry_functions.append_dentry(
                                *current_dir, strdup(component), flags.type,
                                (*current_dir)->inode->sb);
                }
                else if (flags.type == DENTRY_DEVICE)
                {
                    entry = new_dentry_dev(strdup(component), *current_dir,
                                           flags.dev);
                }
            }
            else
            {
                goto not_found;
            }
        }
        else if (flags.create && final_component)
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
    }
    else if (entry->type == DENTRY_FILE)
    {
        if (!final_component)
        {
            goto not_dir;
        }
    }
    else if (entry->type == DENTRY_DEVICE)
    {
        if (!final_component)
        {
            goto not_dir;
        }
    }
    else if (entry->type == DENTRY_MOUNT_POINT)
    {
        /* TODO */
    }
    else if (entry->type == DENTRY_INVALID)
    {
        UNREACHABLE("invalid dentry");
    }
    else
    {
        panic("unhandled dentry type %d in lookup_path", entry->type);
    }

    /* *current_dir = ERR_PTR(-ENOENT); */

out:
    free(component);
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

struct dentry *lookup_path(const char *path, struct pathname_lookup_flags flags)
{
    struct dentry *current_dir = NULL;
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
            current_dir = get_root_dir();
        }
    }
    else
    {
        component_offset++;
        current_dir = get_root_dir();
    }

    while (*(path + component_offset) != 0)
    {

        component_offset +=
            parse_component(path + component_offset, &current_dir, flags);

        /* component_offset++; */

        if (IS_ERR(current_dir))
        {
            return current_dir;
        }
    }

    return current_dir;
}
