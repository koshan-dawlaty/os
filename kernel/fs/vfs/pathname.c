#include "err.h"
#include "vfs.h"
#include "vfs_internal.h"
#include <errno.h>
#include <panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

dentry **pathname_get_dentries(inode *node)
{
    return current_superblock->type->dentry_functions.get_dentries(
        node, current_superblock);
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

dentry *parse_component(dentry **dentries, char *component)
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

dentry *lookup_path(const char *path)
{
    if (*path != '/')
    {
        /* TODO: may have to parse relative paths at some point */
        return ERR_PTR(-EINVAL);
    }

    inode *root_node = pathname_get_root_dir();

    dentry **current_dir = pathname_get_dentries(root_node);
    size_t component_offset = 0;

    size_t j = 0;
    while (path[j] != 0)
    {
        if (path[j] == '/')
        {
            component_offset++;

            size_t component_len = 0;
            while ((path + component_offset)[component_len] != 0 &&
                   (path + component_offset)[component_len] != '/')
            {
                component_len++;
            }

            char *component = malloc(component_len + 1);
            memcpy(component, path + component_offset, component_len);
            component[component_len] = 0;

            component_offset += component_len;

            dentry *entry = parse_component(current_dir, component);
            if (entry == NULL)
            {
                return ERR_PTR(-ENOENT);
            }

            inode *node = entry->inode;

            if (node->type == INODE_DIR)
            {
                current_dir = pathname_get_dentries(node);
            }
            else if (node->type == INODE_FILE)
            {
                return entry;
            }
            else
            {
                panic("unhandled dentry type in lookup_path!!!");
            }
        }
        j++;
    }

    return ERR_PTR(-ENOENT);
}
