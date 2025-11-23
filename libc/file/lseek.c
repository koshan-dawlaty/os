#include "utils.h"
#include <errno.h>
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
    return syscall_wrapper_asm(15, fd, offset, whence, 0, 0);
#endif
}
