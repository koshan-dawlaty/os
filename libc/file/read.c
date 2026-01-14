#include "sys/syscall.h"
#include "utils.h"
#include <errno.h>
#include <kernel_syscall.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __is_libk
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

    if (user_addr_is_valid(buf))
    {
        return -EFAULT;
    }

    file *f = lookup_descriptor(fd);
    return read_file(f, buf, n, f->offset);
#else
    int retval = syscall_wrapper_asm(SYS_read, fd, (uintptr_t)buf, n, 0, 0);

    return retval;
#endif
}

SYSCALL_DEFINE3(read, SYS_read);
