#include "utils.h"
#include <errno.h>
#include <kernel_syscall.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __is_libk
#include <vfs.h>
#endif

off_t lseek(int fd, off_t offset, int whence)
{

    if (fd < 0)
    {
        return -EINVAL;
    }
#ifdef __is_libk

    file *f = lookup_descriptor(fd);
    return seek_file(f, offset, whence);

#else
    return syscall_wrapper_asm(SYS_lseek, fd, offset, whence, 0, 0);
#endif
}

SYSCALL_DEFINE3(SYS_lseek, off_t, int, fd, off_t, offset, int, whence)
{
    return lseek(fd, offset, whence);
}
