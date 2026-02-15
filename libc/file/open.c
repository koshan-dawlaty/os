#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <utils.h>

#ifdef __is_libk
#include <err.h>
#include <vfs.h>
#endif

#include <kernel_syscall.h>

int open(const char *path, int flags)
{
#ifdef __is_libk
    file *f = open_file(path, flags);
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

/* SYSCALL_DEFINE2(SYS_open, int, const char *path, int flags) */
/* { */
/*     return open(path, flags); */
/* } */
