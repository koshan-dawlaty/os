#include "utils.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <kernel_syscall.h>

#ifdef __is_libk
#include <vfs.h>
#endif

int close(int fd)
{
#ifdef __is_libk
    file *f = lookup_descriptor(fd);
    return close_file(f);

#else

    int retval = syscall_wrapper_asm(SYS_close, fd, 0, 0, 0, 0);

    return retval;
#endif
}

SYSCALL_DEFINE1(close, SYS_close);
