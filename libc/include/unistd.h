#ifndef UNISTD_H
#define UNISTD_H
#include "sys/types.h"
#include <stddef.h>

int read(int fd, void *buf, size_t n);
int pread(int fd, void *buf, size_t n, size_t offset);
int execv(const char *path, char *const argv[]);
int sched_yield();
int unlink(const char *name);
int write(int fd, const void *buf, size_t n);
int pwrite(int fd, const void *buf, size_t n, off_t off);
int chdir(const char *path);
unsigned sleep(unsigned int seconds);

#endif
