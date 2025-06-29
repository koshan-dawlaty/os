#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
extern void syscall_wrapper_asm(uint32_t eax, uint32_t ebx, uint32_t ecx,
                                uint32_t edx, uint32_t esi, uint32_t edi);
void putchar2(char c)
{
    putchar(c);
}

void main2()
{
    char test[100];
    char c = 'a';
    while (1)
    {
        putchar2(c);

        /* for (size_t i = 0; i < 0xFFFFFFFFFFFFFFFF; i++) */
        /* { */
        /*     test[i % 100] = i; */
        /* } */
        /* c++; */
    }
}

int main()
{
    main2();
}
