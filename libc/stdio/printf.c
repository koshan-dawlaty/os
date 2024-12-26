#include <kernel/tty.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

double pow(double x, double y)
{
    int i = 0;
    while (i < y)
    {
        x *= x;
        i++;
    }

    return x;
}

int print(const char *str, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        putchar(str[i]);
    }

    return 1;
}

// this could be replaced by a function that returns a char * once
// malloc/kmalloc is implemented
void print_int(intmax_t in, int base)
{

    if (in < 0)
    {
        putchar('-');
        in = -in;
    }

    if (in == 0)
    {
        putchar('0');
        return;
    }

    if ((in / base) > 0)
    {
        print_int(in / base, base);
    }

    intmax_t digit = in % base;
    /* putchar((digit) + 0x30); */
    /* putchar('\n'); */

    if (digit < 10)
    {
        putchar((digit) + 0x30);
    }
    else
    {
        putchar((digit) + (0x37));
    }
}

int printf(const char *format, ...)
{
    va_list parameters;
    va_start(parameters, format);

    int next_is_format = 0;

    while (*format != '\0')
    {

        if (next_is_format)
        {
            next_is_format = 0;

            // may be able to use a switch statement, but there is a chance that
            // they will not work because the bootstrap has not initialized them
            if (*format == 's')
            {
                char *str = va_arg(parameters, char *);

                if (!print(str, strlen(str)))
                {
                    return -1;
                }
            }
            else if (*format == 'c')
            {
                char c = (char)va_arg(parameters, int);

                putchar(c);
            }
            else if (*format == 'd')
            {
                int i = va_arg(parameters, int);
                print_int(i, 10);
            }
            else if (*format == 'x')
            {
                int x = va_arg(parameters, int);
                print_int(x, 16);
            }
            else if (*format == 'p')
            {
                void *p = va_arg(parameters, void *);

                print_int((size_t)p, 16);
            }
        }
        else if (*format == '%')
        {
            next_is_format = 1;
        }
        else
        {
            putchar(*format);
        }

        format++;
    }

    va_arg(parameters, const char *);

    va_end(parameters);

    return 1;
}
