#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern int syscall_wrapper_asm(uint32_t eax, uint32_t ebx, uint32_t ecx,
                               uint32_t edx, uint32_t esi, uint32_t edi);

void test()
{
    asm("nop");
}

void main3()
{
    /* printf("elf_test2 main function called\n"); */

    /* char *a = malloc(10); */
    /* *a = 'a'; */
    /* printf("addr:%p\n", a); */
    /* printf("char:%c\n", *a); */
    /* free(a); */
    /* char *b = malloc(10); */
    /* *b = 'b'; */
    /* printf("addr:%p\n", b); */
    /* printf("char:%c\n", *b); */
    printf("task 2 waiting on semaphore\n");
    syscall_wrapper_asm(5, 1, 0, 0, 0, 0);
    printf("task 2 done waiting on semaphore\n");

    while (1)
    {

        /* putchar('2'); */

        /* putchar('\n'); */
    }
}

int main()
{
    /* *(char *)(0x123456) = 'a'; */
    main3();
}
