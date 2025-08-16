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

    while (1)
    {
    }
}

int main()
{
    main3();
}
