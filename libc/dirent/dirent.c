#include "utils.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __is_libk
#include <vfs.h>
#endif

size_t get_dents(int fd, struct dirent *buf, size_t num)
{
#ifdef __is_libk
    file *f = lookup_descriptor(fd);
    return read_dir(f, buf, num);
#else
    return syscall_wrapper_asm(fd, (uintptr_t)buf, num, 0, 0, 0);
#endif
}

DIR *opendir(const char *name)
{
    DIR *out = malloc(sizeof(DIR));
    out->fd = open(name, 0);
    return out;
}

struct dirent *readdir(DIR *d)
{
    struct dirent *out = malloc(sizeof(struct dirent));
    if (get_dents(d->fd, out, 1) == E2BIG)
    {
        return NULL;
    }

    return out;
}
