#include "lib/log.h"
#include "sys/types.h"
#include <string.h>

void *memset(void *dest, int c, size_t n)
{
    u_char *ptr = (u_char *)dest;

    for (size_t i = 0; i < n; i++)
    {
        ptr[i] = (u_char)c;
    }

    return ptr;
}

void bzero(void *s, size_t n)
{
    memset(s, 0, n);
}
