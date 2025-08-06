#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{

    char *input = malloc(0);
    size_t input_len = 0;

    while (1)
    {

        char c = getchar();

        if (c == '\n')
        {
            execv(input, NULL);
            input = malloc(0);
        }

        if (c == '-')
        {
            c = '_';
        }

        if (c == '\b')
        {

            input[input_len - 1] = 0;
            input_len--;
        }
        else
        {

            input_len++;
            input = realloc(input, input_len);
            input[input_len - 1] = c;
        }

        printf("%s\n", input);
    }
}
