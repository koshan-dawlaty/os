#ifndef FCNTL_H
#define FCNTL_H
#include <sys/types.h>

int open(const char *path, int flags);
int creat(const char *path, mode_t mode);
int unlink(const char *path);

#ifndef CONFIG_HOST_SDL
#define O_EXEC (0 << 1)
#define O_RDONLY (1 << 1)
#define O_RDWR (1 << 2)
#define O_SEARCH (1 << 3)
#define O_WRONLY (1 << 4)
#define O_DIRECTORY (1 << 5)
#define O_CREAT (1 << 6)
#else
#define O_RDONLY 0
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 0x40
#define O_DIRECTORY 0x20000
#endif

#endif
