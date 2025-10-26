#include "string.h"
#include <string.h>

int strcat(char *restrict dest, char *restrict src)
{
    memcpy(dest + strlen(dest), src, strlen(src) + 1);
}
