#include <stdio.h>
#include <string.h>

typedef unsigned char memcpy_t;

void *memcpy(void *dstptr, void *srcptr, size_t size)
{
    memcpy_t *dst = (memcpy_t *)dstptr;
    memcpy_t *src = (memcpy_t *)srcptr;

    for (size_t i = 0; i < size; i++)
    {
        dst[i] = src[i];
    }

    /* printf("memcpy called\n"); */

    return dstptr;
}
