#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#ifdef __is_libk
#include <vfs.h>
#endif

int read(int fd, void *buf, size_t n)
{

    if (fd < 0)
    {
        return -EINVAL;
    }

#ifdef __is_libk
    file *f = lookup_descriptor(fd);
    return read_file(f, buf, n, f->offset);
#else
    int retval = syscall_wrapper_asm(8, fd, (uintptr_t)buf, n, 0, 0);

    return retval;
#endif
}
