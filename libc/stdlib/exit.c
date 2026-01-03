#include <sched.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <utils.h>

#ifdef __is_libk
#include <scheduler.h>
#endif

void exit(int status)
{
#ifdef __is_libk
    struct task *t = get_current_task();
    remove_task(t);
    free_task(t);
    schedule();

#else
    syscall_wrapper_asm(SYS_exit, status, 0, 0, 0, 0);
#endif
}
