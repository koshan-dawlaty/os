#include <errno.h>
#include <fcntl.h>
#include <init.h>
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

    init_libc();

    /* int term_fd = open("/terminal", 0); */
    /* char *buf = malloc(10); */
    /* memcpy(buf, "abc", 4); */
    /* write(term_fd, buf, 10); */

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
                int err = execv(args[0], args);
                if (err < 0)
                {
                    printf("could not execute\nerror:%s", strerror(err));
                    printf("\n");
                }
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
