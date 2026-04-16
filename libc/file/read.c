#include "lib/log.h"
#include "sys/syscall.h"
#include "utils.h"
#include <errno.h>
#include <kernel_syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __is_libk
#include <kernel/syscall.h>
#include <kernel/userspace.h>
#include <vfs.h>
#endif

int read(int fd, void *buf, size_t n)
{

    if (fd < 0)
    {
        return -EINVAL;
    }

#ifdef __is_libk

    struct file *f = lookup_descriptor(fd);
    if (f == NULL)
    {
        return -EBADF;
    }

    return read_file(f, buf, n, f->offset);
#else
    int retval = syscall_wrapper_asm(SYS_read, fd, (uintptr_t)buf, n, 0, 0);

    return retval;
#endif
}

#ifdef __is_libk
SYSCALL_DEFINE3(SYS_read, int, int, fd, void *, buf, size_t, n)
{
    void *kernel = malloc(n);
    size_t retval = read(fd, kernel, n);
    if (copy_to_user(buf, kernel, n))
    {
        return -EFAULT;
    }

    free(kernel);

    return retval;
}

#endif
