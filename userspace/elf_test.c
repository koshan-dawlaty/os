#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

extern void syscall_wrapper_asm(uint32_t eax, uint32_t ebx, uint32_t ecx,
                                uint32_t edx, uint32_t esi, uint32_t edi);

/* int x[10000] = {}; */

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

    putchar('a');
    putchar('b');
    putchar('\n');
    putchar('c');
    putchar('d');

    /* printf("t"); */

    /* syscall_wrapper_asm(1, 2, 300, 4, 5, 6); */
    test2();

    while (1)
    {
    }
}
