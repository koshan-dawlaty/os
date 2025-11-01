#include "sys/types.h"
#include <fcntl.h>

#ifdef __is_libk
#include <err.h>
#include <vfs.h>
#endif

int creat(const char *path, mode_t mode)
{

#ifdef __is_libk
    file *f = create_file(path);
    if (IS_ERR(f))
    {
        return -1;
    }

    return f->descriptor;
#else
    return syscall_wrapper_asm(12, path, mode, 0, 0, 0);
#endif
}
