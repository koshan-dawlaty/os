#include <kernel_syscall.h>
#include <syscall.h>
#include <unistd.h>
#include <utils.h>

#ifdef __is_libk
#include <err.h>
#include <scheduler.h>
#include <vfs.h>
#endif

int chdir(const char *path)
{
#ifdef __is_libk
    struct task *current_task = get_current_task();
    dentry *dir = lookup_path(path, PATHNAME_LOOKUP_FIND);
    printf("new dir is:%p\n", dir);

    if (IS_ERR(dir))
    {
        printf("error:%p\n", GET_ERR(dir));
        return GET_ERR(dir);
    }

    printf("%s\n", dir->name);

    current_task->working_dir = dir;
#else
    return syscall_wrapper_asm(SYS_chdir, (uintptr_t)path, 0, 0, 0, 0);
#endif

    return 0;
}

/* SYSCALL_DEFINE1(SYS_chdir, int, const char *path) */
/* { */
/*     return chdir(path); */
/* } */
