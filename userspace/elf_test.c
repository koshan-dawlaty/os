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
    printf("task 1 waiting on semaphore\n");
    syscall_wrapper_asm(5, 1, 0, 0, 0, 0);
    printf("task 1 done waiting on semaphore\n");

    while (1)
    {
    }
}

int main()
{
    main2();
}
