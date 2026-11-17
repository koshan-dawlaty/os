#include <sys/types.h>
#include <string.h>

void *memset(void *dest, int c, size_t n)
{
    u8 *ptr = (u8 *)dest;

    for (size_t i = 0; i < n; i++)
    {
        ptr[i] = (u8)c;
    }

    return ptr;
}

void bzero(void *s, size_t n)
{
    memset(s, 0, n);
}
