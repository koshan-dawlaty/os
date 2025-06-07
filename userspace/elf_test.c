#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern void syscall_wrapper_asm(uint32_t eax, uint32_t ebx, uint32_t ecx,
                                uint32_t edx, uint32_t esi, uint32_t edi);

/* int x[10000] = {}; */

int global = 4;

void test()
{
    putchar('a');
}

void test2()
{
    test();
}

int main()
{
    /* *(char *)(0x1234) = 'a'; */

    putchar('a');
    putchar('\n');

    /* syscall_wrapper_asm(2, 0, 0, 0, 0, 0); */

    /*     asm("mov $(10), %eax;" */
    /*         "mov $(10), %ebx;" */
    /*         "mov $(10), %ecx;" */
    /*         "mov $(10), %edx;" */
    /*         "mov $(10), %esi;" */
    /*         "mov $(10), %edi;" */

    /*         "int $(0x80)" */

    /*     ); */
    /* asm("int $(0x80)"); */

    /* syscall_wrapper_asm(1, 2, 300, 4, 5, 6); */
    /* syscall_wrapper_asm(0, 'a', 0, 0, 0, 0); */

    /* x[0] = 'a'; */
    /* test2(); */
    /* test2(); */
    /* test2(); */

    /* putchar('a'); */
    /* putchar('b'); */
    /* putchar('\n'); */
    /* /1* putchar('c'); *1/ */
    /* /1* putchar('d'); *1/ */
    /* putchar(global + 0x30); */
    /* putchar('\n'); */
    /* global++; */
    /* putchar(global + 0x30); */
    /* /1* putchar(getchar()); *1/ */

    /* /1* malloc(10); *1/ */

    /* /1* printf("string\n"); *1/ */

    /* /1* syscall_wrapper_asm(1, 2, 300, 4, 5, 6); *1/ */

    while (1)
    {
    }
}
