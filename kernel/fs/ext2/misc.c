#include "ext2.h"
#include "vfs.h"
#include <panic.h>

inode_type ext2_get_inode_type(ext2_inode *node)
{
    size_t type = (node->type >> 12);
    if (type == 4)
    {
        return INODE_DIR;
    }
    else if (type == 8)
    {
        return INODE_FILE;
    }

    panic("unknown inode type:%p\n", type);
}
