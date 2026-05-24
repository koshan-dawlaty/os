#include <lib/utils.h>
#include <string.h>

char *strcpy(char *dest, const char *src)
{
    size_t i;
    for (i = 0; i < strlen(src); i++)
    {
        dest[i] = src[i];
    }
    dest[i] = 0;
    return dest;
}

char *stpcpy(char *dest, const char *src)
{
    size_t i;
    for (i = 0; i < strlen(src); i++)
    {
        dest[i] = src[i];
    }
    dest[i] = 0;
    return dest + i;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < MIN(strlen(src), n); i++)
    {
        dest[i] = src[i];
    }
    dest[i] = 0;
    return dest;
}

char *stpncpy(char *dest, const char *src, size_t n)
{
    size_t i;
    for (i = 0; i < MIN(strlen(src), n); i++)
    {
        dest[i] = src[i];
    }
    return dest + i;
}
