#include "vfs_internal.h"
#include <vfs.h>

file *open_file(char *path)
{

    current_superblock->type->dentry_functions.find_dentry(path,
                                                           current_superblock);
}
