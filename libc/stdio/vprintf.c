#include <extension.h>
#include <kernel/tty.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int print_len(const char *str, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        putchar(str[i]);
    }

    return 1;
}

int print(const char *str)
{
    print_len(str, strlen((char *)str));

    return 1;
}

int vprintf(const char *format, va_list list)
{
    char *str = vrprintf(format, list);
    print(str);
    free(str);
}
