#include <lib/assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __is_libk
#include <panic.h>
#endif

void assert_panic(const char *format, ...)
{
    va_list args;
    va_start(args, format);
#ifdef __is_libk
    vpanic(format, args);
#else
    vprintf(format, args);
    exit(1);
#endif
    va_end(args);
}
