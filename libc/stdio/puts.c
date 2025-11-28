#include <stddef.h>
#include <stdio.h>
#include <string.h>

int puts(char *str)
{

#ifdef __is_libk
    size_t i = 0;
    while (str[i] != 0)
    {
        putchar(str[i]);
        i++;
    }
    return 0;
#else

    /* dont write the null terminator */
    fwrite(str, strlen(str), 1, stdout);
    return 0;
#endif
}
