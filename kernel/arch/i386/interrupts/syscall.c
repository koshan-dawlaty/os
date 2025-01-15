#include <stdint.h>
#include <stdio.h>

struct syscall_regs
{
    uint32_t eax;
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
} __attribute__((packed));

void syscall_handler(struct syscall_regs *registers)
{

    printf("%d\n", registers->eax);

    printf("test\n");
}
