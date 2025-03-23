#ifndef VFS_INTERNAL_H
#define VFS_INTERNAL_H

#include "vfs.h"
#include <stddef.h>

extern superblock *current_superblock;

extern file **open_files;
extern size_t open_files_len;

#endif
