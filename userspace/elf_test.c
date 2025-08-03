#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
extern int syscall_wrapper_asm(uint32_t eax, uint32_t ebx, uint32_t ecx,
                               uint32_t edx, uint32_t esi, uint32_t edi);

void main2()
{

    /* putchar('a'); */
    /* printf("abcdef\n"); */

    /* puts(str); */
    /* putchar(str[0]); */

    /* *(char *)(0x1234) = 'a'; */

    /* test("abcdef%s\n", "12345"); */
    /* test("abcdef\n"); */
    /* printf("test:%s\n", "abcdef"); */
    /* putchar(global); */
    /* puts("puts\n"); */

    /* printf("getchar\n"); */
    /* printf("end\n"); */
    /* char c = getchar(); */
    /* /1* printf("char is:%c\n", c + 0x30); *1/ */

    /* size_t i = 0; */
    /* while (c == 0) */
    /* { */
    /*     c = getchar(); */
    /*     i++; */
    /* } */
    /* printf("i is:%c\n", (char)i + 0x30); */

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

        /* putchar('a'); */
        /* char c = getchar(); */
        /* if (c != 0) */
        /* { */
        /*     printf("char is:%c\n", c); */
        /* } */

        /* putchar('a'); */

        /* printf("task1\n"); */
    }
}

int main()
{
    main2();
}
