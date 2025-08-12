#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    /* free(b); */

    while (1)
    {
        char *b = malloc(10);
        *b = 'b';
        printf("%s", b);
        free(b);

        /* putchar('2'); */

        /* putchar('\n'); */
    }
}

int main()
{
    /* *(char *)(0x123456) = 'a'; */
    main3();
}
