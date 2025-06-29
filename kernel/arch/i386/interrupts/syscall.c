#include "elf.h"
#include "fcntl.h"
#include "kernel/usermode.h"
#include <stdint.h>
#include <stdio.h>

struct syscall_regs
{
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t esi;
    uint32_t edi;
    uint32_t ebp;
    uint32_t padding;
    uint32_t ds;
    uint32_t es;
    uint32_t fs;
    uint32_t gs;
    uint32_t eax;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;

} __attribute__((packed));

/* int test; */

void syscall_handler(struct syscall_regs *registers)
{

    /* *(char *)(0xC03FF002 + test) = 'a'; */
    /* test += 2; */
    /*
printf("eax: %p, ebx:%p, ecx:%p, edx:%p, esi:%p, edi:%p, ebp:%p\n",
       registers->eax, registers->ebx, registers->ecx, registers->edx,
       registers->esi, registers->edi, registers->ebp);
               */

    if (registers->eax == 0)
    {
        /* printf("test\n"); */
        /* putchar(registers->ebx); */
        *(char *)(0xC03FF000) = registers->ebx;
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
