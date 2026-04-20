#ifndef FCNTL_H
#define FCNTL_H
#include "sys/types.h"

/*
 * TODO: flags are ignored
 */
int open(const char *path, int flags);
int creat(const char *path, mode_t mode);
int unlink(const char *path);

#define O_EXEC (0 << 1)
#define O_RDONLY (1 << 1)
#define O_RDWR (1 << 2)
#define O_SEARCH (1 << 3)
#define O_WRONLY (1 << 4)
#define O_DIRECTORY (1 << 5)

#endif
