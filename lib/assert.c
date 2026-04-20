#include <lib/assert.h>
#include <lib/log.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __is_libk
#include <panic.h>
#endif

void __panic_func(const char *format, ...)
{
    va_list args;
    va_start(args, format);
#ifdef __is_libk
    vpanic(format, args);
#else
    vprintf(format, args);
    vlog_level(LOG_ERROR, format, args);
    exit(1);
#endif
    va_end(args);
}
