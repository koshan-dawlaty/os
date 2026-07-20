#include "lib/log.h"
#ifdef __is_libk
#include <kernel/userspace.h>
#include <scheduler.h>
#include <vfs.h>
#endif

#include <kernel_syscall.h>
#include <lib/utils.h>
#include <sys/syscall.h>
#include <syscall.h>
#include <types.h>
#include <unistd.h>
#include <utils.h>

char *getcwd(char *buf, size_t size)
{
#ifdef __is_libk
    struct dentry *entry = get_current_task()->working_dir;
    if (entry == NULL)
    {
        copy_to_user(buf, "/", MAX(2, size));
    }
    else
    {
        /* FIXME: dentry_get_pathname() should probably copy_to_user(), but it
         * would be nice to use it for kernel debugging */
        dentry_get_pathname(entry, buf, size);
    }
    return buf;
#else
    return (char *)(ptr_t)syscall_wrapper_asm(SYS_getcwd, (ptr_t)buf, size, 0,
                                              0, 0);
#endif
}

SYSCALL_DEFINE2(SYS_getcwd, ptr_t, char *, buf, size_t, size)
{
    return (ptr_t)getcwd(buf, size);
}
