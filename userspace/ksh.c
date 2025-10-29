#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef uintptr_t ptr;
extern int syscall_wrapper_asm(ptr ax, ptr bx, ptr cx, ptr dx, ptr si, ptr di);

int main()
{

    /* putchar('a'); */
    /* syscall_wrapper_asm(12, 'a', 0, 0, 0, 0); */

    char *input = malloc(0);
    size_t input_len = 0;

    while (1)
    {
        char c = getchar();

        if (c == '\n')
        {

            input = realloc(input, strlen(input) + 2);
            input[strlen(input)] = ' ';

            char **args = malloc(sizeof(char *));
            size_t j = 0;

            size_t i = 0;
            size_t current_arg_start = 0;

            while (input[i] != 0)
            {

                if (input[i] == ' ')
                {
                    j++;
                    args = realloc(args, sizeof(char *) * j);
                    args[j - 1] = malloc(i - current_arg_start + 2);
                    args[j - 1][i - current_arg_start] = 0;

                    memcpy(args[j - 1], input + current_arg_start,
                           i - current_arg_start);

                    current_arg_start = i + 1;

                    goto end;
                }

            end:
                i++;
            }

            args = realloc(args, sizeof(char *) * (j + 1));
            args[j] = NULL;

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
            input = realloc(input, input_len);
            input[input_len - 1] = c;
        }

        /* for (size_t i = 0; i < input_len + 2; i++) */
        /* { */
        /*     putchar('\b'); */
        /* } */
        /* putchar('\n'); */

        putchar('\b');

        printf(">%s\n", input);
    }
}
