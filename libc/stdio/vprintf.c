#include <kernel/tty.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

// TODO: kinda slow
char *int_to_string(intmax_t in, int base)
{

    int negative = 0;
    if (in < 0)
    {
        negative = 1;
        in *= -1;
    }

    size_t digits_len = 0;
    intmax_t in_temp = in;
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
        intmax_t digit = in % base;
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
    return buf;
}

int vprintf(const char *format, va_list list)
{

    int next_is_format = 0;

    while (*format != '\0')
    {

        if (next_is_format)
        {
            next_is_format = 0;

            // may be able to use a switch statement, but there is a chance
            // that they will not work because the bootstrap has not
            // initialized them
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
                int i = va_arg(list, int);
                print(int_to_string(i, 10));
            }
            else if (*format == 'x')
            {
                int x = va_arg(list, int);
                print(int_to_string(x, 16));
            }
            else if (*format == 'p')
            {
                void *p = va_arg(list, void *);
                print(int_to_string((uint32_t)p, 16));
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

    va_end(list);

    return 1;
}
