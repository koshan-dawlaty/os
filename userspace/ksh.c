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
            free(input);
            input = malloc(0);
            input_len = 0;
            continue;
        }

        if (c == '-')
        {
            c = '_';
        }

        if (c == '\b' && input_len != 0)
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

        /* for (size_t i = 0; i < input_len + 2; i++) */
        /* { */
        /*     putchar('\b'); */
        /* } */
        /* putchar('\n'); */

        printf("\033[2J>%s\n", input);
    }
}
