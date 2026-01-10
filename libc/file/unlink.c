#include "errno.h"
#include <fcntl.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <utils.h>
#include <kernel_syscall.h>

#ifdef __is_libk
#include <vfs.h>
#endif

int unlink(const char *path)
{
#ifdef __is_libk
    dentry *entry = lookup_path(path, PATHNAME_LOOKUP_FIND);
    return delete_dentry(entry);
#else
    return syscall_wrapper_asm(SYS_unlink, (uintptr_t)path, 0, 0, 0, 0);
#endif
}
SYSCALL_DEFINE1(unlink, SYS_unlink);
