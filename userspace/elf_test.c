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

    while (1)
    {
        char *a = malloc(10);
        *a = 'a';
        printf("%s", a);
        free(a);
    }
}

int main()
{
    main2();
}
