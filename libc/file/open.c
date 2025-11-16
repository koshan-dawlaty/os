#include "utils.h"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <utils.h>

#ifdef __is_libk
#include <err.h>
#include <vfs.h>
#endif

int open(char *path, int flags)
{
#ifdef __is_libk
    file *f = open_file(path);
    if (IS_ERR(f))
    {
        /* TODO: we might want to set errno here */
        return -GET_ERR(f);
    }

    return f->descriptor;
#else
    int fd = syscall_wrapper_asm(7, (uintptr_t)path, flags, 0, 0, 0);
    if (fd < 0)
    {
        errno = -fd;
        return -1;
    }
    return fd;
#endif
}
