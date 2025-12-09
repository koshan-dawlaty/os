#include "kernel/graphics.h"
#include "memory/get_free_pages.h"
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <interrupts/idt.h>
#include <kernel/paging.h>
#include <kernel/semaphore.h>
#include <kernel/types.h>
#include <memory/paging_flags.h>
#include <panic.h>
#include <scheduler.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils.h>

/* eax should be at 19 words offset */
struct syscall_regs
{
    ptr ebx;
    ptr ecx;
    ptr edx;
    ptr esi;
    ptr edi;
    ptr ebp;

    ptr fs;
    ptr gs;
    ptr eax;
    ptr ip;
    ptr cs;
    ptr flags;
    ptr sp;
    ptr ss;

} __attribute__((packed));

/* int test; */
semaphore *test_semaphore = NULL;

int syscall_handler(struct syscall_regs *regs)
{

    /* *(char *)(0xC03FF002 + test) = 'a'; */
    /* test += 2; */
    /* printf("eax: %p, ebx:%p, ecx:%p, edx:%p, esi:%p, edi:%p, ebp:%p\n", */
    /*        regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi,
     */
    /*        regs->ebp); */

    switch (regs->eax)
    {
    case 0:
        putchar(regs->ebx);
        return 0;
    case 1:
    {
        /* asm("sti"); */
        char c = getchar();
        /* printf("in syscall:%c\n", c); */
        return c;
    }
    case 2:
    {
        return execv((void *)regs->ebx, (void *)regs->ecx);
    }
    case 3:
    {
        /* void *ip_address; */
        /* asm("lea (%%eip), %0" : "=r"(ip_address)); */

        struct interrupt_frame f = {regs->ip, regs->cs, regs->flags, regs->sp,
                                    regs->ss};

        /* struct registers r = (struct registers){ */
        /*     regs->gs,  regs->fs,  regs->es, regs->ds,  regs->edi, */
        /*     regs->esi, regs->ebp, regs->sp, regs->ebx, regs->edx, */
        /*     regs->ecx, regs->eax, f}; */

        schedule();

        /* switch_tasks(r); */
        return 1;
    }
    case 4:
    {
        for (size_t i = 0; i < regs->ecx / 4096; i++)
        {
            map_page(get_free_phys_page(), (void *)regs->ebx + (i * 4096),
                     PAGING_WRITEABLE | PAGING_USER | PAGING_PRESENT);
        }

        return 1234;
    }
    case 5:
    {
        wait_semaphore(test_semaphore);
        getchar();
        release_semaphore(test_semaphore);
        return 1234;
    }
    case 6:
    {
        return pread(regs->ebx, (void *)regs->ecx, regs->edx, regs->edi);
    }
    case 7:
    {
        return open((void *)regs->ebx, 0);
    }
    case 8:
    {
        return read(regs->ebx, (void *)regs->ecx, regs->edx);
    }
    case 9:
    {
        exit(regs->ebx);
        return 1234;
    }
    case 10:
    {
        return write(regs->ebx, (void *)regs->ecx, regs->edx);
    }
    case 11:
    {
        return pwrite(regs->ebx, (void *)regs->ecx, regs->edx, regs->esi);
    }
    case 12:
    {
        /* TODO: this should be implemented with an open flag instead */
        return creat((void *)regs->ebx, regs->ecx);
    }
    case 13:
    {
        return chdir((void *)regs->ebx);
    }
    case 14:
    {
        return unlink((char *)regs->ebx);
    }
    case 15:
    {
        return lseek(regs->ebx, regs->ecx, regs->edx);
    }
    case 16:
    {
        return close(regs->ebx);
    }
    case 17:
    {
        int retval = get_dents(regs->ebx, (void *)regs->ecx, regs->edx);

        return retval;
    }
    case 18:
        /* TODO: this should probably be in a device file, not a syscall */
        *(struct framebuffer **)regs->ebx =
            framebuffer_alloc_user(get_current_framebuffer());

        return 0;
    }

    printf("no syscall found\n");
    return -ENOSYS;
}
