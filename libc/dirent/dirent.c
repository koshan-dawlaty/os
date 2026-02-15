#include "utils.h"
#include <dirent.h>
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
    file *f = lookup_descriptor(fd);
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
/* SYSCALL_DEFINE3(SYS_getdents, long, int fd, struct dirent *buf, size_t num)
 */
/* { */
/*     return get_dents(fd, buf, num); */
/* } */

DIR *opendir(const char *name)
{
    DIR *out = malloc(sizeof(DIR));
    out->fd = open(name, 0);
    return out;
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
