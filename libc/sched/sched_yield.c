#include <kernel_syscall.h>
#include <utils.h>
#include <unistd.h>
#ifdef __is_libk
#include <scheduler.h>
#endif

int sched_yield()
{
#ifdef __is_libk
    schedule();
#else
    syscall_wrapper_asm(3, 0, 0, 0, 0, 0);
#endif
    return 0;
}

SYSCALL_DEFINE0(3, int)
{
    sched_yield();

    return 0;
}
