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

void print_panic_user(const char *format, va_list args)
{
    static char format_data[4096];
    vsnprintf(format_data, 4096, format, args);
    loge(USER_PANIC_MESSAGE "%s", format_data);
    printf(USER_PANIC_MESSAGE "%s", format_data);
}

void __panic_func(const char *format, ...)
{
    va_list args;
    va_start(args, format);
#ifdef __is_libk
    vpanic(format, args);
#else
    print_panic_user(format, args);
    exit(1);
#endif
    va_end(args);

    while (1)
    {
        asm("hlt"); /* will probably cause a gpf if run in userspace, which is
                       kind of what we want if exit() does not work */
    }
}
