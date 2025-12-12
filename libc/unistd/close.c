#include "utils.h"
#include <sys/syscall.h>
#include <unistd.h>

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
