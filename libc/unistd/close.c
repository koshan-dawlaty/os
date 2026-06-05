#include "errno.h"
#include "utils.h"
#include <kernel_syscall.h>
#include <sys/syscall.h>
#include <unistd.h>

#ifdef __is_libk
#include <vfs.h>
#endif

int close(int fd)
{
#ifdef __is_libk
    if (fd < 0)
    {
        return -EBADF;
    }
    struct file *f = lookup_descriptor(fd);
    if (f != NULL)
    {
        int retval = close_file(f);
        fdlist_close(get_current_fdlist(), fd);
        return retval;
    }
    else
    {
        return -EBADF;
    }

#else

    int retval = syscall_wrapper_asm(SYS_close, fd, 0, 0, 0, 0);

    return retval;
#endif
}

SYSCALL_DEFINE1(SYS_close, int, int, fd)
{
    return close(fd);
}
