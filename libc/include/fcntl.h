#ifndef FCNTL_H
#define FCNTL_H
#include "sys/types.h"

/*
 * TODO: flags are ignored
 */
int open(const char *path, int flags);
int creat(const char *path, mode_t mode);
int unlink(const char *path);

#endif
