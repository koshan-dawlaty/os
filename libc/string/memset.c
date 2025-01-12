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
