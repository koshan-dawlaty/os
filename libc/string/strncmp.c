#include <string.h>

int strncmp(const char *s1, const char *s2, size_t n)
{

    /* if (strlen((char *)s1) != strlen((char *)s2)) */
    /* { */
    /*     return 1; */
    /* } */

    size_t i = 0;
    while (s1[i] != '\0' && i < n)
    {
        if (s1[i] != s2[i])
        {
            return 1;
        }
        i++;
    }

    if (i > n)
    {
        return 1;
    }

    return 0;
}
