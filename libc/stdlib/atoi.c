#include "lib/log.h"
#include <stdlib.h>

int char_to_int(char in)
{
    return in - '0';
}

int atoi(char *in)
{
    int i = 0;
    int output = 0;
    while (in[i] != 0)
    {
        i++;
    }

    i--;
    while (i >= 0)
    {

        output += char_to_int(in[i]) * (10 * (i + 1));

        i--;
    }

    return output;
}
