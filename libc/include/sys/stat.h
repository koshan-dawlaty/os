#ifndef SYS_STAT_H
#define SYS_STAT_H

#include <fcntl.h>
#include <sys/types.h>

int mkdir(const char *path, mode_t mode);

struct stat;
int stat(const char *pathname, struct stat *buf);
struct stat
{
    off_t st_size;
    mode_t mode;
};

#define S_IFMT (-1)

#define S_IFREG (1 << 0)
#define S_IFDIR (1 << 1)

#define S_IS_REG(mode) (mode & S_IS_REG)
#define S_IS_DIR(mode) (mode & S_IS_DIR)

#endif
