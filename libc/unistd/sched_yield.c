#include <utils.h>

void sched_yield()
{
    syscall_wrapper_asm(3, 0, 0, 0, 0, 0);
}
