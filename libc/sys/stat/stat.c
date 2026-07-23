#include "lib/log.h"
#include <err_ptr.h>
#include <kernel_syscall.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <syscall.h>
#include <utils.h>

#include <kernel/userspace.h>
#ifdef __is_libk
#include <vfs.h>
#endif

int stat(const char *pathname, struct stat *buf)
{
#ifdef __is_libk
    struct dentry *ent = lookup_path(pathname, PATHNAME_LOOKUP_FIND);
    if (IS_ERR(ent))
    {
        return GET_ERR(ent);
    }

    buf->mode = 0;

    switch (ent->type)
    {
        /* FIXME: this wont work for devices, as they are DENTRY_FILE */
    case DENTRY_FILE:

        buf->mode |= S_IFREG;
        break;
    case DENTRY_DIR:
    case DENTRY_MOUNT_POINT:

        buf->mode |= S_IFDIR;
        break;
    case DENTRY_INVALID:
    case DENTRY_SOCKET:
        logw("unimplemented stat dentry type %d\nfor pathname:%s", ent->type,
             pathname);
        break;
    }
    return 0;

#else
    return syscall_wrapper_asm(SYS_stat, (ptr_t)pathname, (ptr_t)buf, 0, 0, 0);
#endif
}

SYSCALL_DEFINE2(SYS_stat, int, const char *, pathname, struct stat *, buf)
{
    char *kpath = get_user_pathname(pathname);
    int retval = stat(kpath, buf);
    free(kpath);
    return retval;
}
