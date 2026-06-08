#include "lib/log.h"
#include <stdlib.h>

int powi(int x, int y)
{
    int out = 1;
    for (int i = 0; i < y; i++)
    {
        out *= x;
    }
    return out;
}

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
    size_t j = 0;
    while (i >= 0)
    {

        int num = char_to_int(in[i]);
        output += num * powi(10, j);

        j++;
        i--;
    }

    return output;
}
