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

    return kernel_syscall(regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi,
                          regs->edi);

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
    case SYS_execv:
    {
        return execv((void *)regs->ebx, (void *)regs->ecx);
    }
    case 3:
    {
    }
    case SYS_mmap:
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
    case SYS_pread:
    {
        return pread(regs->ebx, (void *)regs->ecx, regs->edx, regs->edi);
    }
    case SYS_open:
    {
        return open((void *)regs->ebx, regs->ecx);
    }
    case SYS_read:
    {
        return read(regs->ebx, (void *)regs->ecx, regs->edx);
    }
    case SYS_exit:
    {
        exit(regs->ebx);
        return 1234;
    }
    case SYS_write:
    {
        return write(regs->ebx, (void *)regs->ecx, regs->edx);
    }
    case SYS_pwrite:
    {
        return pwrite(regs->ebx, (void *)regs->ecx, regs->edx, regs->esi);
    }
    case SYS_creat:
    {
        /* TODO: this should be implemented with an open flag instead */
        return creat((void *)regs->ebx, regs->ecx);
    }
    case 13:
    {
        return chdir((void *)regs->ebx);
    }
    case SYS_unlink:
    {
        return unlink((char *)regs->ebx);
    }
    case SYS_lseek:
    {
        return lseek(regs->ebx, regs->ecx, regs->edx);
    }
    case SYS_close:
    {
        return close(regs->ebx);
    }
    case SYS_getdents:
    {
        int retval = get_dents(regs->ebx, (void *)regs->ecx, regs->edx);

        return retval;
    }
    case 18:
        /* TODO: this should probably be in a device file, not a syscall */
        *(struct framebuffer **)regs->ebx =
            framebuffer_alloc_user(get_current_framebuffer());

        return 0;
    case SYS_create_socket:
    {
        struct unix_socket *s = unix_socket_alloc();
        return unix_socket_add_array(s);
    }

    case SYS_send:
    {
        struct unix_socket *s = unix_socket_lookup(regs->ebx);
        return unix_socket_send(s, (void *)regs->ecx, regs->edx);
    }

    case SYS_recv:
    {
        struct unix_socket *s = unix_socket_lookup(regs->ebx);
        return unix_socket_recv(s, (void *)regs->ecx, regs->edx);
    }
    case SYS_clone:
        struct task *new = user_thread_alloc(regs->ebx);
        add_task(new);

        return 0;
    }

    printf("no syscall found\n");
    return -ENOSYS;
}
