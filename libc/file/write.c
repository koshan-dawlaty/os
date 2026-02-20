#include "sys/types.h"
#include "utils.h"
#include <errno.h>
#include <kernel_syscall.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#ifdef __is_libk
#include <kernel/userspace.h>
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

#ifdef __is_libk

SYSCALL_DEFINE3(SYS_write, int, int, fd, const void *, buf, size_t, n)
{
    void *kernel = malloc(n);
    if (copy_from_user(kernel, buf, n))
    {
        return -EFAULT;
    }
    int retval = write(fd, kernel, n);
    free(kernel);

    return retval;
}

SYSCALL_DEFINE4(SYS_pwrite, int, int, fd, const void *, buf, size_t, n, off_t,
                off)
{
    void *kernel = malloc(n);
    if (copy_from_user(kernel, buf, n))
    {
        return -EFAULT;
    }

    int retval = pwrite(fd, kernel, n, off);
    free(kernel);

    return retval;
}

#endif
