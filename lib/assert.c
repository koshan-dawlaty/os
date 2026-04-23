#include <extension.h>
#include <lib/assert.h>
#include <lib/log.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __is_libk
#include <panic.h>
#endif

#define USER_PANIC_MESSAGE                                                     \
    "userspace program panicked\n"                                             \
    "reason:\n"

void __panic_func(const char *format, ...)
{
    va_list args;
    va_start(args, format);
#ifdef __is_libk
    vpanic(format, args);
#else
    char *str = vrprintf(format, args);
    loge("%s", str);
    printf("%s", str);
    exit(1);
#endif
    va_end(args);
}
