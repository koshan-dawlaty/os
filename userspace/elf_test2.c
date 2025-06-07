#include <stdio.h>

void test()
{
    asm("nop");
}

int main()
{
    /* *(char *)(0x123456) = 'a'; */
    while (1)
    {
        /* test(); */
    }
    putchar('b');
    while (1)
    {
    }
}
