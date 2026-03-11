#include "utils.h"
#include <err_ptr.h>
#include <kernel_syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <syscall.h>

#ifdef __is_libk
#include <err.h>
#include <vfs.h>
#endif

int mkdir(const char *path, mode_t mode)
{
#ifdef __is_libk

    struct dentry *new_dir = lookup_path(
        path, (struct pathname_lookup_flags){.create = 1, .type = DENTRY_DIR});

    if (IS_ERR(new_dir))
    {
        return -GET_ERR(new_dir);
    }
    return 0;
#else
    return syscall_wrapper_asm(SYS_mkdir, (uintptr_t)path, mode, 0, 0, 0);
#endif
}

SYSCALL_DEFINE2(SYS_mkdir, int, const char *, path, mode_t, mode)
{
    return mkdir(path, mode);
}
