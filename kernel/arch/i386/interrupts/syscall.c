#include "elf.h"
#include "endian.h"
#include "fcntl.h"
#include "interrupts/idt.h"
#include "kernel/keyboard.h"
#include "kernel/paging.h"
#include "kernel/usermode.h"
#include "memory/paging_flags.h"
#include "scheduler.h"
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

int syscall_handler(struct syscall_regs *regs)
{

    /* *(char *)(0xC03FF002 + test) = 'a'; */
    /* test += 2; */
    /* printf("eax: %p, ebx:%p, ecx:%p, edx:%p, esi:%p, edi:%p, ebp:%p\n", */
    /*        registers->eax, registers->ebx, registers->ecx, registers->edx, */
    /*        registers->esi, registers->edi, registers->ebp); */

    if (regs->eax == 0)
    {
        /* printf("test\n"); */
        /* putchar(regs->ebx); */
        *(char *)(0xC03FF100) = regs->ebx;
        return 0;
    }
    else if (regs->eax == 1)
    {
        /* asm("sti"); */
        char c = getchar();
        printf("in syscall:%c\n", c);
        return c;
    }
    else if (regs->eax == 2)
    {

        int f = open((char *)regs->ebx, 0);

        elf_32 *elf = parse_elf32(f);
        if (elf != NULL)
        {
            add_task(load_elf32(elf));
        }

        /* printf("syscall\n"); */
        return 0;
        /* jump_usermode_asm(0x60017c); */
        /* jump_usermode_asm(elf->entry_point); */
    }
    else if (regs->eax == 3)
    {
        /* void *ip_address; */
        /* asm("lea (%%eip), %0" : "=r"(ip_address)); */

        struct interrupt_frame f = {regs->ip, regs->cs, regs->flags, regs->sp,
                                    regs->ss};

        /* struct registers r = (struct registers){ */
        /*     regs->gs,  regs->fs,  regs->es, regs->ds,  regs->edi, */
        /*     regs->esi, regs->ebp, regs->sp, regs->ebx, regs->edx, */
        /*     regs->ecx, regs->eax, f}; */

        /* switch_tasks(r); */
        return 1;
    }
    else if (regs->eax == 4)
    {
        printf("mmap addr:%p\n", regs->ebx);
        map_page(get_free_phys_page(), (void *)regs->ebx,
                 PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT);

        return 1234;
    }
    else if (regs->eax == 5)
    {
    }

    printf("no syscall found\n");
    /* should  be ENOSYS */
    return -0x12345;

    /* printf("test\n"); */
}
