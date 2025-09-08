#include <unistd.h>

#ifdef __is_libk
#include <err.h>
#include <scheduler.h>
#include <vfs.h>
#endif

int chdir(const char *path)
{
#ifdef __is_libk
    struct task *current_task = get_current_task();
    dentry *dir = lookup_path(path);
    printf("new dir is:%p\n", dir);

    if (IS_ERR(dir))
    {
        printf("error:%p\n", GET_ERR(dir));
        return GET_ERR(dir);
    }

    printf("%s\n", dir->name);

    current_task->working_dir = dir;
#else
    return syscall_wrapper_asm(13, path, 0, 0, 0, 0);
#endif
}
