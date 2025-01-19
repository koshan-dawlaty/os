#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *strdup(char *in)
{
    size_t in_len = strlen(in);
    char *out = malloc((in_len + 1) * sizeof(char));

    for (size_t i = 0; i < in_len + 1; i++)
    {
        out[i] = in[i];
    }

    return out;
}
