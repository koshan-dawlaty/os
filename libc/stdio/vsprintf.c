// this should maybe be vsnprintf
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int vsprintf(char *s, const char *format, va_list list)
{

    int buffer_index = 0;
    uint8_t next_is_format = 0;

    while (*format != '\0')
    {

        if (*format == '%')
        {
            next_is_format = 1;
        }
        else if (next_is_format)
        {
            if (*format == 's')
            {
                char *s2 = va_arg(list, char *);

                for (size_t i = 0; i < strlen(s2); i++)
                {
                    s[buffer_index] = s2[i];
                    buffer_index++;
                }
            }

            else if (*format == 'c')
            {
                char c = (char)va_arg(list, int);

                s[buffer_index] = c;
                buffer_index++;
            }
            else if (*format == 'd')
            {
                int i = va_arg(list, int);
                char *s2 = int_to_string(i, 10);

                for (size_t i = 0; i < strlen(s2); i++)
                {
                    s[buffer_index] = s2[i];
                    buffer_index++;
                }
            }
            else if (*format == 'x')
            {
                int x = va_arg(list, int);
                char *s2 = int_to_string(x, 16);

                for (size_t i = 0; i < strlen(s2); i++)
                {
                    s[buffer_index] = s2[i];
                    buffer_index++;
                }
            }
            else if (*format == 'p')
            {
                void *p = va_arg(list, void *);
                char *s2 = int_to_string((uint32_t)p, 16);

                for (size_t i = 0; i < strlen(s2); i++)
                {
                    s[buffer_index] = s2[i];
                    buffer_index++;
                }
            }

            next_is_format = 0;
        }
        else
        {
            s[buffer_index] = *format;
            buffer_index++;
        }

        format++;
    }
}
