#include <errno.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <utils.h>

#ifdef __is_libk
#include "scheduler.h"
#endif
#include <kernel_syscall.h>

/*
 * TODO: does not sleep for the specified number of seconds, it instead sleeps
 * for scheduler ticks, i should probably just use usleep()
 */
unsigned sleep(unsigned int seconds)
{
#ifdef __is_libk
    add_sleep_queue(get_current_task(), seconds);
#else
    syscall_wrapper_asm(SYS_sleep, seconds, 0, 0, 0, 0);
#endif

    /* TODO: return value */
    return -EINVAL;
}

#ifdef __is_libk

SYSCALL_DEFINE1(SYS_sleep, int, unsigned int, seconds)
{
    sleep(seconds);
    return 0;
}

#endif
