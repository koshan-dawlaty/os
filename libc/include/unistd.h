#ifndef UNISTD_H
#define UNISTD_H
#include <stddef.h>

int read(int fd, char *buf, size_t n);
int pread(int fd, char *buf, size_t n, size_t offset);
int execv(const char *path, char *const argv[]);

#endif
