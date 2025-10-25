#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
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

            input_len++;
            input = realloc(input, input_len);
            input[input_len - 1] = 0;

            char *s = strtok(input, " ");
            char **args = malloc(sizeof(char *));

            size_t i = 0;
            while (s != NULL)
            {
                i++;
                args = realloc(args, sizeof(char *) * i);
                args[i - 1] = s;

                s = strtok(NULL, " ");
            }

            i++;
            args = realloc(args, sizeof(char *) * (i));
            args[i - 1] = NULL;

            if (!strcmp(args[0], "cd"))
            {
                if (args[1] != NULL)
                {
                    chdir(args[1]);
                }
                else
                {
                    printf("not enough arguments\n");
                }
            }
            else
            {
                execv(args[0], args);
            }
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
            input = realloc(input, input_len + 1);
            input[input_len - 1] = c;
            input[input_len] = 0;
        }

        putchar('\b');

        printf(">%s\n", input);
    }
}
