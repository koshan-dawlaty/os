#include <kernel/tty.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int __attribute__((format(printf, 1, 2))) printf(const char *format, ...)
{
    va_list parameters;
    va_start(parameters, format);

    vprintf(format, parameters);

    va_end(parameters);

    return 1;
}
