#include <sched.h>
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

    while (1)
    {

        putchar('1');
        /* sched_yield(); */
        /* char c = getchar(); */
        /* putchar(c); */
        /* printf("char is:%c\n", c); */
        /* if (c != 0) */
        /* { */
        /* } */

        /* putchar('a'); */

        /* printf("task1\n"); */
    }
}

int main()
{
    main2();
}
