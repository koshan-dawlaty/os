#include "fcntl.h"
#include "lib/log.h"
#include <err_ptr.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <utils.h>

#ifdef __is_libk
#include <err.h>
#include <kernel/userspace.h>
#include <vfs.h>
#endif

#include <kernel_syscall.h>

int open(const char *path, int flags)
{
#ifdef __is_libk
    int fd;
    struct file *f = open_file(path, flags, &fd);
    if (IS_ERR(f))
    {
        int err = GET_ERR(f);
        if (err == ENOENT && (flags & O_CREAT))
        {
            struct file *f = create_file((char *)path);
            if (IS_ERR(f))
            {
                return -1;
            }
            int retval = fdlist_add(get_current_fdlist(), f);

            return retval;
        }
        else
        {
            /* TODO: we might want to set errno here */
            return -GET_ERR(f);
        }
    }

    return fd;
#else
    int fd = syscall_wrapper_asm(SYS_open, (uintptr_t)path, flags, 0, 0, 0);
    if (fd < 0)
    {
        errno = -fd;
        return -1;
    }
    return fd;
#endif
}

#ifdef __is_libk
SYSCALL_DEFINE2(SYS_open, int, const char *, path, int, flags)
{

    char *kpath = get_user_pathname(path);
    int retval = open(kpath, flags);
    free(kpath);

    return retval;
}

#endif
