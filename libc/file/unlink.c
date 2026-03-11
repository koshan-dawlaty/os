#include <fcntl.h>
#include <kernel_syscall.h>
#include <scheduler.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <utils.h>

#include <err_ptr.h>
#ifdef __is_libk
#include <errno.h>
#include <vfs.h>
#endif

int unlink(const char *path)
{
#ifdef __is_libk
    struct dentry *entry = lookup_path(path, PATHNAME_LOOKUP_FIND);
    if (IS_ERR(entry))
    {
        return -GET_ERR(entry);
    }
    return delete_dentry(entry);
#else
    return syscall_wrapper_asm(SYS_unlink, (uintptr_t)path, 0, 0, 0, 0);
#endif
}

SYSCALL_DEFINE1(SYS_unlink, int, const char *, path)
{
    return unlink(path);
}
