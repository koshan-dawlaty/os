#include "stdio.h"

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/tty.h>

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

// TODO: kinda slow
char *int_to_string(intmax_t in, int base)
{
    if (in == 0)
    {

        char *buf = malloc(2);
        buf[0] = '0';
        buf[1] = '\0';

        return buf;
    }

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
    buf[buf_size] = 0;
    return buf;
}

const size_t buf_increment = 10;

void increment_buf(char **buf, size_t *len, size_t *capacity, size_t add)
{

    if ((*len + add + 1) > *capacity)
    {
        *capacity += *len + add + buf_increment;
        *buf = realloc(*buf, *capacity + 1);
    }

    *len += add;
}

void append_buf(char **buf, size_t *buf_len, size_t *capacity, char new)
{
    increment_buf(buf, buf_len, capacity, 1);
    (*buf)[*buf_len - 1] = new;
    (*buf)[*buf_len] = 0;
}

/* not standard */
char *vrprintf(const char *format, va_list list)
{

    char *out = malloc(buf_increment);
    out[0] = 0;
    /* total bytes */
    size_t out_len = 0;
    size_t capacity = 10;

    size_t i = 0;

    int next_is_format = 0;

    if (*format == 0)
    {
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

                increment_buf(&out, &out_len, &capacity, strlen(str));
                strcat(out, str);
            }
            else if (*format == 'c')
            {
                char c = (char)va_arg(list, int);

                append_buf(&out, &out_len, &capacity, c);
            }
            else if (*format == 'd')
            {
                int i = va_arg(list, int);
                char *str = int_to_string(i, 10);

                increment_buf(&out, &out_len, &capacity, strlen(str));
                strcat(out, str);
            }
            else if (*format == 'x')
            {
                int x = va_arg(list, int);
                char *str = int_to_string(x, 16);

                increment_buf(&out, &out_len, &capacity, strlen(str));
                strcat(out, str);
            }
            else if (*format == 'p')
            {
                void *p = va_arg(list, void *);
                char *str = int_to_string((uint32_t)p, 16);

                increment_buf(&out, &out_len, &capacity, strlen(str));
                strcat(out, str);
            }
        }
        else if (*format == '%')
        {
            next_is_format = 1;
        }
        else
        {
            append_buf(&out, &out_len, &capacity, *format);
        }

        format++;
    }

    append_buf(&out, &out_len, &capacity, 0);

    return out;
}
