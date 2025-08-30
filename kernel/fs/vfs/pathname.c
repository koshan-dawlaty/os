#include "err.h"
#include "vfs.h"
#include "vfs_internal.h"
#include <errno.h>
#include <panic.h>
#include <stdio.h>
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

dentry *lookup_path(const char *path)
{
    if (*path != '/')
    {
        /* TODO: may have to parse relative paths at some point */
        return ERR_PTR(-EINVAL);
    }

    dentry *retval = NULL;

    inode *root_node = pathname_get_root_dir();

    dentry **current_dir = pathname_get_dentries(root_node);
    size_t component = 0;

    size_t j = 0;
    while (path[j] != 0)
    {
        if (path[j] == '/')
        {
            component++;

            size_t i = 0;

            while (current_dir[i] != NULL)
            {
                const char *component_str = path + component;
                char *name = current_dir[i]->name;
                size_t strcmp_len = strlen(component_str) < strlen(name)
                                        ? strlen(component_str)
                                        : strlen(name);

                if (!strncmp(name, path + component, strcmp_len))

                {
                    component = i;
                    /* TODO free this */
                    inode *temp_node =
                        pathname_get_inode(current_dir[i]->inode->number);
                    /* check if it is a directory */

                    if (temp_node->type == INODE_DIR)
                    {
                        current_dir = pathname_get_dentries(temp_node);
                    }
                    else if (temp_node->type == INODE_FILE)
                    {
                        return current_dir[i];
                    }
                    else
                    {
                        panic("unhandled inode type in pathname lookup\n");
                    }

                    if (path[j + 1] == 0)
                    {
                        retval = current_dir[i];
                        goto end;
                    }

                    goto loop_end;
                }
                i++;
            }
            return ERR_PTR(-ENOENT);
        loop_end:;
        }
        j++;
    }

end:

    return retval;
}
