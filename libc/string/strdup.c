#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *strdup(char *in)
{
    size_t in_len = strlen(in);
    char *out = malloc(in_len + 1);

    for (size_t i = 0; i < in_len + 1; i++)
    {
        out[i] = in[i];
    }

    return out;
}

char *strndup(const char *in, size_t num)
{
    size_t len = strlen(in);
    char *out = malloc(len + 1);
    for (size_t i = 0; i < len + 1 && i < num; i++)
    {
        out[i] = in[i];
    }
    return out;
}
