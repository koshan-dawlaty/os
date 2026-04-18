#include "stdio.h"
#include "sys/types.h"
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __is_libk
#include <kernel/tty.h>
#endif

// TODO: kinda slow
char *int_to_string_unsigned(uintmax_t in, long base)
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

    char *buf = malloc(buf_size);

    size_t i = digits_len;

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

    if ((*len + add + 1) >= *capacity)
    {
        while ((*len + add + 1) > *capacity)
        {
            *capacity += buf_increment;
        }

        *buf = realloc(*buf, *capacity);
    }
}

void append_buf(char **buf, size_t *buf_len, size_t *capacity, char new)
{
    increment_buf(buf, buf_len, capacity, 1);
    (*buf_len)++;
    (*buf)[*buf_len - 1] = new;
    (*buf)[*buf_len] = 0;
}

/* not standard */
char *vrprintf(const char *format, va_list list)
{
    /* puts((char *)format); */
    /* puts("\n"); */

    char *out = malloc(buf_increment);

    /* total bytes */
    size_t out_len = 0;
    size_t capacity = 10;

    bool is_unsigned = false;

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

                if (str == NULL)
                {
                    str = "null";
                }

                increment_buf(&out, &out_len, &capacity, strlen(str));

                out_len += strlen(str);
                memcpy(out + (out_len - strlen(str)), str, strlen(str));
            }
            else if (*format == 'c')
            {
                char c = (char)va_arg(list, int);

                append_buf(&out, &out_len, &capacity, c);
            }
            else if (*format == 'u')
            {
                is_unsigned = true;
                next_is_format = true;
            }
            else
            {
                char *str;
                if (is_unsigned)
                {
                    if (*format == 'd')
                    {
                        uint i = va_arg(list, uint);
                        str = int_to_string_unsigned(i, 10);
                    }
                    else if (*format == 'x')
                    {
                        uint i = va_arg(list, uint);
                        str = int_to_string_unsigned(i, 16);
                    }
                    else if (*format == 'L')
                    {
                        unsigned long long i = va_arg(list, unsigned long long);
                        str = int_to_string_unsigned(i, 16);
                    }
                    else if (*format == 'l')
                    {
                        unsigned long long i = va_arg(list, unsigned long);
                        str = int_to_string_unsigned(i, 10);
                    }
                    else
                    {
                        /* TODO: error */
                        str = "";
                    }
                    is_unsigned = false;
                }
                else
                {
                    if (*format == 'd')
                    {
                        int i = va_arg(list, int);
                        str = int_to_string(i, 10);
                    }
                    else if (*format == 'x' || *format == 'X')
                    {
                        int x = va_arg(list, int);
                        str = int_to_string(x, 16);
                    }
                    else if (*format == 'z')
                    {
                        size_t x = va_arg(list, size_t);
                        str = int_to_string(x, 10);
                    }
                    else if (*format == 'p')
                    {
                        void *x = va_arg(list, void *);
                        str = int_to_string_unsigned((uintptr_t)x, 16);
                    }
                    else
                    {

                        /* TODO: error */
                        /* str = malloc(3); */
                        /* str[0] = '%'; */
                        /* str[1] = *format; */
                        /* str[2] = 0; */

                        str = malloc(2);
                        /* str[0] = *format; */
                        str[0] = '-';
                        str[1] = 0;

                        str = "";

                        /* TODO: this is the case where we dont know what format
                         * string was passed, however for the acpica code, their
                         * logging printf function uses weird format specifers
                         * that i dont want to support right now, so just assume
                         * that there will be one at some point */

                        next_is_format = 1;
                    }
                }

                increment_buf(&out, &out_len, &capacity, strlen(str));

                out_len += strlen(str);
                memcpy(out + (out_len - strlen(str)), str, strlen(str));
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

    /* this line is where the malloc bug is caused */
    append_buf(&out, &out_len, &capacity, 0);

    return out;
}

char *rprintf(char *format, ...)
{
    va_list args;
    va_start(args, format);

    char *buf = vrprintf(format, args);

    va_end(args);

    return buf;
}
