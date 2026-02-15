#include "kernel/syscall.h"
#include "kernel/graphics.h"
#include "memory/get_free_pages.h"
#include "socket.h"
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
#include <sys/stat.h>
#include <sys/syscall.h>
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

    if (regs->eax == SYS_get_framebuffer)
    {
        *(struct framebuffer **)regs->ebx =
            framebuffer_alloc_user(get_current_framebuffer());

        return 1;
    }

    return kernel_syscall(regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi,
                          regs->edi);
}
