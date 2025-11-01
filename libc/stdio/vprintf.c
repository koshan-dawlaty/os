#include <extension.h>
#include <kernel/tty.h>
#include <kernel/types.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* seems to use SSE registers on x86_64, and it does not seem to be used
 * anywhere */
/*
double pow(double x, double y)
{
    int i = 0;
    while (i < y)
    {
        x *= x;
        i++;
    }

    return x;
} */

void print(const char *str)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        putchar(str[i]);
    }
}

int vprintf(const char *format, va_list list)
{
    char *str = vrprintf(format, list);
    print(str);
    free(str);
}
