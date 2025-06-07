#include "elf.h"
#include "fcntl.h"
#include "kernel/usermode.h"
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

/* int test; */

void syscall_handler(struct syscall_regs *registers)
{

    /* *(char *)(0xC03FF002 + test) = 'a'; */
    /* test += 2; */
    /* printf("eax: %d, ebx:%d, ecx:%d, edx:%d, esi:%d, edi:%d, ebp:%d\n", */
    /*        registers->eax, registers->ebx, registers->ecx, registers->edx, */
    /*        registers->esi, registers->edi, registers->ebp); */

    if (registers->eax == 0)
    {
        /* printf("test\n"); */
        putchar(registers->ebx);
    }
    else if (registers->eax == 1)
    {
        registers->ebx = getchar();
    }
    else if (registers->eax == 2)
    {

        int f = open("/usr/bin/elf_test2", 0);
        elf_32 *elf = parse_elf32(f);
        load_elf32(elf);
        printf("syscall\n");
        /* jump_usermode_asm(0x60017c); */
        /* jump_usermode_asm(elf->entry_point); */
    }

    /* printf("test\n"); */
}
