#include "dirent.h"
#include "lib/log.h"
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
#include <syscall.h>
#include <unistd.h>

char current_dir[100];

void print_prompt()
{
    printf("\n%s > ", current_dir);
}

int main()
{

    getcwd(current_dir, sizeof(current_dir));
    print_prompt();

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
                    int retval = chdir(args[1]);
                    logd("%d", retval);
                    if (retval != 0)
                    {
                        printf("cant cd, err:%s\n", strerror(retval));
                    }
                }
                else
                {
                    printf("not enough arguments\n");
                }
                getcwd(current_dir, sizeof(current_dir));
                print_prompt();
            }
            else
            {
                char *program = strdup(args[0]);
                const char *bin_dir = "/bin/";
                char *full_path =
                    calloc(strlen(program) + strlen(bin_dir) + 1, 1);

                strcat(full_path, bin_dir);
                strcat(full_path, program);

                int err = execv(full_path, args);
                if (err < 0)
                {
                    printf("could not execute\nerror:%s", strerror(err));
                    /* printf("\n"); */
                }
                /* TODO: this should have a way of waiting for the process to
                 * exit */
                sleep(2);
                print_prompt();
            }
            free(input);
            input = malloc(0);
            input_len = 0;
            continue;
        }

        /* if (c == '-') */
        /* { */
        /*     c = '_'; */
        /* } */

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

        /* putchar(c); */

        /* for (size_t i = 0; i < input_len + 1; i++) */
        /* { */
        /*     putchar('\b'); */
        /* } */
        /* printf("%s, %c\n", input, c); */
        /* printf(">%s", input); */
    }
}
