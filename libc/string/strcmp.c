#include <string.h>

int strcmp(const char *s1, const char *s2)
{
    if (strlen((char *)s1) != strlen((char *)s2))
    {
        return 1;
    }

    size_t i = 0;
    while (s1[i] != '\0')
    {
        if (s1[i] != s2[i])
        {
            return 1;
        }
        i++;
    }

    return 0;
}
