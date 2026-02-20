#include <kernel/faults.h>
#include <utils.h>
#include <kernel/scheduler.h>


void page_fault(void * addr, struct page_fault_flags flags) {
    if (!flags.user) {
        return; /* TODO: right now we expect the architecture code to handle stopping the kernel and printing the addr, but that should be the responsibility of this function */
    }
    IRQ_OFF({
        struct task * t = get_current_task();
        
        printf("user page fault at addr:%p\n", addr); /* TODO: make this print to the task's stdout */
        
        remove_task(t);
    })
    
}