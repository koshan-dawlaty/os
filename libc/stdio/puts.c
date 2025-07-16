#include <stddef.h>
#include <stdio.h>

int puts(char *str)
{
    size_t i = 0;
    while (str[i] != 0)
    {
        putchar(str[i]);
        i++;
    }
    return 0x12345;
}
