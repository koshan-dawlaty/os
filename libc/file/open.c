#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <utils.h>

#ifdef __is_libk
#include <err.h>
#include <vfs.h>
#endif

int open(char *path, int flags)
{
#ifdef __is_libk
    file *f = open_file(path);
    if (IS_ERR(f))
    {
        return -1;
    }

    return f->descriptor;
#else
    return syscall_wrapper_asm(7, (uintptr_t)path, flags, 0, 0, 0);
#endif
}
