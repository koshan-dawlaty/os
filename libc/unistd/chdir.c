#include "errno.h"
#include "lib/log.h"
#include <kernel_syscall.h>
#include <syscall.h>
#include <unistd.h>
#include <utils.h>

#include <err_ptr.h>
#ifdef __is_libk
#include <scheduler.h>
#include <vfs.h>
#endif

int chdir(const char *path)
{
#ifdef __is_libk
    struct task *current_task = get_current_task();
    struct dentry *dir = lookup_path(path, PATHNAME_LOOKUP_FIND);
    if (dir->type != DENTRY_DIR && dir->type != DENTRY_MOUNT_POINT)
    {
        return -ENOTDIR;
    }
    /* printf("new dir is:%p\n", dir); */

    if (IS_ERR(dir))
    {
        return GET_ERR(dir);
    }
    /* logd("new dir:%s", dir->name); */

    /* printf("%s\n", dir->name); */

    current_task->working_dir = dir;
#else
    return syscall_wrapper_asm(SYS_chdir, (uintptr_t)path, 0, 0, 0, 0);
#endif

    return 0;
}

SYSCALL_DEFINE1(SYS_chdir, int, const char *, path)
{
    return chdir(path);
}
