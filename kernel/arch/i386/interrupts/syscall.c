#include "elf.h"
#include "endian.h"
#include "fcntl.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/usermode.h"
#include "memory/paging_flags.h"
#include <stdint.h>
#include <stdio.h>
#include <utils.h>

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

int syscall_handler(struct syscall_regs *registers)
{

    /* *(char *)(0xC03FF002 + test) = 'a'; */
    /* test += 2; */
    /* printf("eax: %p, ebx:%p, ecx:%p, edx:%p, esi:%p, edi:%p, ebp:%p\n", */
    /*        registers->eax, registers->ebx, registers->ecx, registers->edx, */
    /*        registers->esi, registers->edi, registers->ebp); */

    if (registers->eax == 0)
    {
        /* printf("test\n"); */
        putchar(registers->ebx);
        /* *(char *)(0xC03FF000) = registers->ebx; */
        return 0;
    }
    else if (registers->eax == 1)
    {
        /* registers->ebx = pop_keyboard_buffer(); */
        char c = pop_keyboard_buffer();
        if (c != 0)
        {
            /* printf("in syscall:%p\n", c); */
        }

        /* printf("waiting\n"); */
        /* while (1) */
        /* { */
        /* } */

        return c;
    }
    else if (registers->eax == 2)
    {

        int f = open((char *)registers->ebx, 0);

        elf_32 *elf = parse_elf32(f);
        if (elf != NULL)
        {
            load_elf32(elf);
        }
        /* printf("syscall\n"); */
        return 0;
        /* jump_usermode_asm(0x60017c); */
        /* jump_usermode_asm(elf->entry_point); */
    }
    else if (registers->eax == 3)
    {
    }
    else if (registers->eax == 4)
    {
        printf("mmap addr:%p\n", registers->ebx);
        map_page(get_free_phys_page(), (void *)registers->ebx,
                 PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT);

        return 1234;
    }
    else if (registers->eax == 5)
    {
    }

    printf("no syscall found\n");
    /* should  be ENOSYS */
    return -0x12345;

    /* printf("test\n"); */
}
