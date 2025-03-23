// TODO: needs have a POSIX compliant name
#ifndef FILE_H
#define FILE_H

#include <stddef.h>

/*
 * TODO: flags are ignored
 */
int open(char *path, int flags);

int read(int fd, char *buf, size_t n);

#endif
