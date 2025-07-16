#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
extern void syscall_wrapper_asm(uint32_t eax, uint32_t ebx, uint32_t ecx,
                                uint32_t edx, uint32_t esi, uint32_t edi);

int global = 0;

void main2()
{

    printf("abcdef\n");

    /* puts(str); */
    /* putchar(str[0]); */

    /* *(char *)(0x1234) = 'a'; */

    /* test("abcdef%s\n", "12345"); */
    /* test("abcdef\n"); */
    printf("test:%s\n", "abcdef");
    /* putchar(global); */
    /* puts("puts\n"); */

    while (1)
    {

        /* printf("task1\n"); */
    }
}

int main()
{
    main2();
}
