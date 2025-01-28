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
char *int_to_string2(intmax_t in, int base)
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
    // TODO: THIS WILL BREAK!!!!
    char *buffer = malloc(100);
    vsprintf(buffer, format, list);

    while (*buffer != '\0')
    {
        putchar(*buffer);
        buffer++;
    }

    free(buffer);
}
