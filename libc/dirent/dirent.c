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

long get_dents(int fd, struct dirent *buf, size_t num)
{
    long retval;
#ifdef __is_libk
    file *f = lookup_descriptor(fd);
    retval = read_dir(f, buf, num);
#else
    retval = syscall_wrapper_asm(17, fd, (uintptr_t)buf, num, 0, 0);
#endif

    if (retval < 0)
    {
        errno = -retval;
        return -1;
    }
    return retval;
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
    if (get_dents(d->fd, out, 1) == -1)
    {
        if (errno == EINVAL)
        {
            /* should be done reading dirs */
            return NULL;
        }
        else
        {
            /* TODO: error */
        }
    }

    return out;
}
