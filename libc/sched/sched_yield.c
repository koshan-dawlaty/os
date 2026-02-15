#include <kernel_syscall.h>
#include <utils.h>
#ifdef __is_libk
#include <scheduler.h>
#endif

void sched_yield()
{
#ifdef __is_libk
    schedule();
#else
    syscall_wrapper_asm(3, 0, 0, 0, 0, 0);
#endif
}

/* SYSCALL_DEFINE0(sched_yield, 3) */
