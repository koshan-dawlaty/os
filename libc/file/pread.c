#include <errno.h>
#include <kernel_syscall.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <utils.h>

#if defined(__is_libk)
#include <vfs.h>
#endif

int pread(int fd, void *buf, size_t n, size_t offset)
{
#if defined(__is_libk)
    struct file *f = lookup_descriptor(fd);
    if (f == NULL)
    {
        return -EBADF;
    }
    return read_file(f, buf, n, offset);
#else
    return syscall_wrapper_asm(SYS_pread, fd, (uintptr_t)buf, n, offset, 0);
#endif
}

SYSCALL_DEFINE4(SYS_pread, int, int, fd, void *, buf, size_t, n, size_t, offset)
{
    return pread(fd, buf, n, offset);
}
