#include <kernel_syscall.h>
#include <lib/log.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <utils.h>

#ifdef __is_libk
#include <scheduler.h>
#include <vfs.h>
#endif

void exit(int status)
{
#ifdef __is_libk
	logi("process exited with code %d", status);
    struct task *t = get_current_task();

    /* printf("exit addr_space:%p\n", &t->addr_space); */
    /* addr_space_free(t->addr_space); */
    task_remove_free(t);

#else
    syscall_wrapper_asm(SYS_exit, status, 0, 0, 0, 0);
#endif
}

SYSCALL_DEFINE1(SYS_exit, int, int, status)
{
    exit(status);
    return 0;
}
