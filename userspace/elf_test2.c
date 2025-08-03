#include <stdio.h>

void test()
{
    asm("nop");
}

void main3()
{
    printf("elf_test2 main function called\n");
    while (1)
    {

        /* putchar('\n'); */
    }
}

int main()
{
    /* *(char *)(0x123456) = 'a'; */
    main3();
}
