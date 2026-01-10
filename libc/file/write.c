#include "sys/types.h"
#include "utils.h"
#include <errno.h>
#include <kernel_syscall.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#ifdef __is_libk
#include <vfs.h>
#endif

int write(int fd, const void *buf, size_t n)
{
    if (fd < 0)
    {
        return -EINVAL;
    }

#ifdef __is_libk
    file *f = lookup_descriptor(fd);
    return write_file(f, buf, n, f->offset);
#endif

    return syscall_wrapper_asm(SYS_write, fd, (uintptr_t)buf, n, 0, 0);
}

int pwrite(int fd, const void *buf, size_t n, off_t off)

{
    if (fd < 0)
    {
        return -EINVAL;
    }

#ifdef __is_libk
    file *f = lookup_descriptor(fd);
    return write_file(f, (void *)buf, n, off);
#endif

    return syscall_wrapper_asm(SYS_pwrite, fd, (uintptr_t)buf, n, off, 0);
}

SYSCALL_DEFINE3(write, SYS_write);
SYSCALL_DEFINE4(pwrite, SYS_pwrite);
