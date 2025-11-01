#include "ext2.h"
#include "vfs.h"
#include <panic.h>

dentry_type ext2_get_dentry_type(ext2_inode *node)
{
    size_t type = (node->type >> 12);
    if (type == 4)
    {
        return DENTRY_DIR;
    }
    else if (type == 8)
    {
        return DENTRY_FILE;
    }
    else
    {
        return DENTRY_INVALID;
    }
}
