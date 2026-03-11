#include "sys/types.h"
#include <err_ptr.h>
#include <fcntl.h>
#include <kernel_syscall.h>
#include <sys/syscall.h>

#include <utils.h>

#ifdef __is_libk
#include <err.h>
#include <panic.h>
#include <vfs.h>
#endif

int creat(const char *path, mode_t mode)
{

#ifdef __is_libk
    file *f = create_file((char *)path);
    if (IS_ERR(f))
    {
        return -1;
    }

    return f->descriptor;
#else
    return syscall_wrapper_asm(SYS_creat, (uintptr_t)path, mode, 0, 0, 0);
#endif
}
SYSCALL_DEFINE2(SYS_creat, int, const char *, path, mode_t, mode)
{
    return creat(path, mode);
}
