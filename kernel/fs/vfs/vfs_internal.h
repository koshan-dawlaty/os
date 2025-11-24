#ifndef VFS_INTERNAL_H
#define VFS_INTERNAL_H

#include "vfs.h"
#include <stddef.h>

extern superblock *current_superblock;

/* this is only here so the file functions will still work when the scheduler
 * is off (need task->descriptors for lookup_descriptor) */
extern struct fdlist *basic_open_files;

#endif
