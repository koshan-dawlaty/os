#include <errno.h>
#include <unistd.h>

#ifdef __is_libk
#include "scheduler.h"
#endif

/*
 * TODO: does not sleep for the specified number of seconds, it instead sleeps
 * for scheduler ticks, i should probably just use usleep()
 */
unsigned sleep(unsigned int seconds)
{
#ifdef __is_libk
    add_sleep_queue(get_current_task(), seconds);
#else
/* TODO: syscall */
#endif

    /* TODO: return value */
    return -EINVAL;
}
