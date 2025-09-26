#include <kernel/tty.h>
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

char *int_to_string_unsigned(uintmax_t in, uintmax_t base, bool negative)
{
    if (in == 0)
    {

        char *buf = malloc(2);
        buf[0] = '0';
        buf[1] = '\0';

        return buf;
    }

    size_t digits_len = 0;
    uintmax_t in_temp = in;
    while (in_temp > 0)
    {
        digits_len++;
        in_temp /= base;
    }

    size_t buf_size = digits_len * sizeof(char);
    if (negative)
    {
        buf_size++;
    }

    char *buf = malloc(buf_size);

    size_t i = digits_len;
    if (negative)
    {
        buf[0] = '-';
        i++;
    }

    /* printf("%c\n", buf[0]); */
    while (in > 0)
    {
        uintmax_t digit = in % base;
        /* printf("in:%p, digit:%p\n", (void *)in, (void *)digit); */
        if (digit < 10)
        {
            buf[i - 1] = digit + 0x30;
        }
        else
        {
            buf[i - 1] = digit + 0x37;
        }

        /* printf("i: %d\n", i); */
        /* printf("%d\n", in % 10); */
        /* printf("%c\n", (in % 10) + 0x30); */
        i--;
        in /= base;
    }

    /* printf("%c\n", buf[0]); */
    /* printf("test:%c\n", (buf[0] == '\0') + 0x30); */
    buf[buf_size] = 0;
    return buf;
}

// TODO: kinda slow
char *int_to_string(intmax_t in, int base)
{
    if (in < 0)
    {
        return int_to_string_unsigned(in, base, true);
    }

    return int_to_string_unsigned(in, base, false);
}

int vprintf(const char *format, va_list list)
{

    int next_is_format = 0;

    if (*format == 0)
    {

        putchar('a');
    }

    while (*format != '\0')
    {

        if (next_is_format)
        {
            next_is_format = 0;

            /*
may be able to use a switch statement, but there is a chance
that they will not work because the bootstrap has not initialized them
            */
            if (*format == 's')
            {

                char *str = va_arg(list, char *);

                if (!print_len(str, strlen(str)))
                {
                    return -1;
                }
            }
            else if (*format == 'c')
            {
                char c = (char)va_arg(list, int);

                putchar(c);
            }
            else if (*format == 'd')
            {
                long i = va_arg(list, int);
                print(int_to_string(i, 10));
            }
            else if (*format == 'x')
            {
                long long x = va_arg(list, int);
                print(int_to_string(x, 16));
            }
            else if (*format == 'p')
            {
                uint64_t p = va_arg(list, uint64_t);
                print(int_to_string_unsigned((uint64_t)p, 16, false));
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

    va_arg(list, const char *);

    return 1;
}
