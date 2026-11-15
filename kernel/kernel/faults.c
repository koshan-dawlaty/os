#include <kernel/faults.h>
#include <kernel/stacktrace.h>
#include <lib/log.h>
#include <panic.h>
#include <scheduler.h>
#include <stdio.h>
#include <utils.h>

static void print_page_fault_flags(struct page_fault_flags flags)
{
    char user = flags.user ? 'u' : '-';
    char instruction = flags.instruction ? 'i' : '-';
    char present = !flags.not_present ? 'p' : '-';
    char other = flags.other ? 'o' : '-';

    printf("%c%c%c%c", user, instruction, present, other);
}

extern void syscall_handler_page_fault_stub(ptr_t sp);

void page_fault(void *addr, void *ip, struct page_fault_flags flags)
{
    struct task *t = get_current_task();

    ASSERT(!(t->checking_user_addr && flags.user),
           "checking_user_addr set while task is in userspace, the kernel "
           "probably didn't call user_addr_check_end() after a "
           "user_addr_check_start()");

    if (t->checking_user_addr)
    {
        t->checking_user_addr = false;
        syscall_handler_page_fault_stub((ptr_t)t->user_syscall_start_stack);
        return;
    }
    if (!flags.user)
    {
        printf("page fault\n");

        loge("page fault\naddr:%p\nip:%p", addr, ip);

        printf("addr:%p\n", addr);
        print_page_fault_flags(flags);
        printf("\n");
        printf("instruction pointer:%p\n", ip);

        stacktrace_log();
        while (true)
        {
            irq_disable();
            irq_wait();
        }
    }
    IRQ_OFF({
        /* TODO: make this print to the task's stdout */
        loge("user page fault at addr:%p", addr);
        printf("user page fault at addr:%p\n", addr);
        print_page_fault_flags(flags);
        printf("\n");
        while (true)
        {
            irq_disable();
            irq_wait();
        }

        task_remove_free(t);
    })
}
