#include <lib/log.h>
#include <string.h>

char *strcat(char *restrict dest, const char *restrict src)
{
    memcpy(dest + strlen(dest), src, strlen(src) + 1);

    return dest;
}
