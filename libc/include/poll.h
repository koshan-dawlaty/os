#ifndef LIBC_POLL_H
#define LIBC_POLL_H

#include <stddef.h>
#include <sys/syscall.h>
#include <utils.h>

int poll(int *fds, size_t num)
{
    /* TODO: error check (maybe) */
    return syscall_wrapper_asm(SYS_poll, (ptr)fds, num, 0, 0, 0);
}

#endif
