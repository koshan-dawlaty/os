#include "assert.h"
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

u16 ext2_get_inode_type(dentry_type type)
{
    switch (type)
    {
    case DENTRY_DIR:
        return EXT2_INODE_DIR;
    case DENTRY_FILE:
        return EXT2_INODE_FILE;

    default:
        UNREACHABLE("only files and dentries are supported by ext2");
    }
}
