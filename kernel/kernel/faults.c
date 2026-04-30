#include "lib/log.h"
#include "panic.h"
#include <kernel/faults.h>
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

void page_fault(void *addr, void *ip, struct page_fault_flags flags)
{
    if (!flags.user)
    {
        printf("page fault\n");
        loge("page fault\naddr:%p\nip:%p", addr, ip);

        printf("addr:%p\n", addr);
        print_page_fault_flags(flags);
        printf("\n");
        printf("instruction pointer:%p\n", ip);
        while (true)
        {
            irq_disable();
            irq_wait();
        }
    }
    IRQ_OFF({
        struct task *t = get_current_task();

        loge("user page fault at addr:%p",
             addr); /* TODO: make this print to the task's stdout */
        printf("user page fault at addr:%p\n",
               addr); /* TODO: make this print to the task's stdout */
        print_page_fault_flags(flags);
        printf("\n");
        while (1)
        {
        }

        /* TODO: free task (maybe make a separate function to remove and free at
         * the same time */

        remove_task(t);
    })
}
