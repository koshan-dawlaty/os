#include <string.h>

void *memcpy(void *dstptr, void *srcptr, size_t size) {
    char *dst = (char *)dstptr;
    char *src = (char *)srcptr;

    for (size_t i = 0; i < size; i++) {
        dst[i] = src[i];
    }

    return dstptr;
}
