#include "faults.h"
#include "idt_internal.h"

#include "interrupts/idt.h"

#include <stdint.h>
#include <stdio.h>

extern int get_cr2();

void fault_handler_wait()
{
    while (1)
    {
        asm("hlt");
    }
}

ISR(gpf_handler, {
    printf("gpf!!!\n");

    fault_handler_wait();
});

ISR(default_fault_handler, {
    printf("default fault handler called\n");

    fault_handler_wait();
});

ISR(double_fault_handler, {
    printf("double fault!\n");

    fault_handler_wait();
});

ISR(page_fault_handler, {
    printf("page fault!\n");

    printf("faulting address:%p\n", (void *)get_cr2());
    printf("error code:%p\n", (void *)frame->flags);
    fault_handler_wait();
});

ISR(virtualization_fault_handler, {
    printf("virtualization exception\n");
    fault_handler_wait();
});

ISR(division_fault_handler, {
    printf("divide by zero!\n");
    fault_handler_wait();
});

void init_faults()
{
    for (uint8_t i = 0; i < 32; i++)
    {
        add_idt_entry((IDT_entry){(uint32_t)default_fault_handler, 8, 0x8F}, i);
    }

    add_idt_entry((IDT_entry){(uint32_t)gpf_handler, 8, 0x8F}, 0x0D);
    add_idt_entry((IDT_entry){(uint32_t)double_fault_handler, 8, 0x8F}, 0x08);
    add_idt_entry((IDT_entry){(uint32_t)page_fault_handler, 8, 0x8F}, 0x0E);
    add_idt_entry((IDT_entry){(uint32_t)virtualization_fault_handler, 8, 0x8F},
                  0x20);
}
