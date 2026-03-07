#ifndef EXTENSION_H
#define EXTENSION_H
/*
 * everything in this file is not standard
 */

#include "sys/syscall.h"
#include "utils.h"
#include <stdarg.h>

/*
 * version of vsprintf that uses a heap pointer that it reallocs
 */
char *vrprintf(const char *format, va_list list);
char *rprintf(char *format, ...);

static inline int user_mutex_create()
{
    return syscall_wrapper_asm(SYS_mutex_create, 0, 0, 0, 0, 0);
}

static inline void user_mutex_lock(int mutex)
{
    syscall_wrapper_asm(SYS_mutex_lock, mutex, 0, 0, 0, 0);
}

static inline void user_mutex_unlock(int mutex)
{
    syscall_wrapper_asm(SYS_mutex_unlock, mutex, 0, 0, 0, 0);
}

#endif
