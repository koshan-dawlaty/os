#include "sys/types.h"
#include "utils.h"
#include <errno.h>
#include <unistd.h>
#ifdef __is_libk
#include <vfs.h>
#endif

int write(int fd, void *buf, size_t n)
{
    if (fd < 0)
    {
        return -EINVAL;
    }

#ifdef __is_libk
    file *f = lookup_descriptor(fd);
    return write_file(f, buf, n, f->offset);
#endif

    return syscall_wrapper_asm(10, fd, (uint32_t)buf, n, 0, 0);
}

int pwrite(int fd, void *buf, size_t n, off_t off)
{
    if (fd < 0)
    {
        return -EINVAL;
    }

#ifdef __is_libk
    file *f = lookup_descriptor(fd);
    return write_file(f, buf, n, off);
#endif

    return syscall_wrapper_asm(11, fd, (uint32_t)buf, n, off, 0);
}
