#include "utils.h"
#include <dirent.h>
#include <err_ptr.h>
#include <errno.h>
#include <fcntl.h>
#include <kernel_syscall.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __is_libk
#include <vfs.h>
#endif

long get_dents(int fd, struct dirent *buf, size_t num)
{
#ifdef __is_libk
    struct file *f = lookup_descriptor(fd);
    if (f == NULL)
    {
        return -EBADF;
    }
    /* printf("file:%p\n", f); */
    /* printf("fd:%d\n", fd); */
    long retval = read_dir(f, buf, num);
    return retval;
#else
    long retval =
        syscall_wrapper_asm(SYS_getdents, fd, (uintptr_t)buf, num, 0, 0);
    return retval;
#endif
}
SYSCALL_DEFINE3(SYS_getdents, long, int, fd, struct dirent *, buf, size_t, num)
{
    return get_dents(fd, buf, num);
}

DIR *opendir(const char *name)
{
    int fd = open(name, O_DIRECTORY);
    if (fd < 0)
    {
        return NULL;
    }
    DIR *out = malloc(sizeof(DIR));
    out->fd = fd;

    return out;
}

DIR *fdopendir(int fd)
{
    if (fd < 0)
    {
        return NULL;
    }
    DIR *out = malloc(sizeof(DIR));
    out->fd = fd;
    return out;
}

int closedir(DIR *d)
{
    close(d->fd);
    free(d);
    return 0;
}

struct dirent *readdir(DIR *d)
{
    errno = 0;
    struct dirent *out = malloc(sizeof(struct dirent));

    int err = get_dents(d->fd, out, 1);

    if (err < 0)
    {
        if (-err != EINVAL)
        {
            errno = -err;
        }
        return NULL;
    }

    return out;
}
