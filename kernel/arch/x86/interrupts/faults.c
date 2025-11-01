#include "faults.h"
#include "idt_internal.h"

#include "interrupts/idt.h"
#include "kernel/types.h"

#include <stdint.h>
#include <stdio.h>

extern int get_cr2();

void fault_handler_wait()
{
    asm("cli");
    while (1)
    {
        asm("hlt");
    }
}

ISR(gpf_handler, {
    printf("gpf!!!\n");
    printf("cs:%p\n", (void *)frame->cs);
    printf("error code:%p\n", (void *)frame->flags);
    printf("ip:%p\n", (void *)frame->ip);
    printf("sp:%p\n", (void *)frame->sp);
    printf("ss:%p\n", frame->ss);

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

/* TODO: make this a macro in idt.h like the ISR marco but with error code
 * parameter */
_Pragma("clang diagnostic push");
_Pragma("clang diagnostic ignored \"-Wexcessive-regsave\"");
ISR_ATTRIBUTE void page_fault_handler(struct interrupt_frame *frame,
                                      unsigned long int error_code)
{ /* this is to prevent the unused parameter warning */
    (void)frame;

    printf("page fault\n");
    printf("addr:%p\n", get_cr2());
    printf("error code:%p\n", error_code);
    printf("instruction pointer:%p\n", frame->ip);
    fault_handler_wait();
}
_Pragma("clang diagnostic pop");

ISR(virtualization_fault_handler, {
    printf("virtualization exception\n");
    fault_handler_wait();
});

ISR(division_fault_handler, {
    printf("divide by zero\n");
    fault_handler_wait();
});
ISR(tss_fault_handler, {
    printf("invalid tss\n");
    fault_handler_wait();
});
ISR(invalid_opcode, {
    printf("invalid opcode\n");
    fault_handler_wait();
});

ISR(coprocesser_segment_overrun, {
    printf("coprocesser_segment_overrun\n");
    fault_handler_wait();
})

void init_faults()
{
    const u8 idt_fault_flags = 0x8F;

    for (uint8_t i = 0; i < 32; i++)
    {
        add_idt_entry(
            (IDT_entry){(ptr)default_fault_handler, 8, idt_fault_flags, 0}, i);
    }

    add_idt_entry((IDT_entry){(ptr)gpf_handler, 8, idt_fault_flags, 0}, 0x0D);

    /* add_idt_entry((IDT_entry){(ptr)coprocesser_segment_overrun, 8,
     * idt_fault_flags, 0}, 9); */

    add_idt_entry((IDT_entry){(ptr)invalid_opcode, 8, idt_fault_flags, 0},
                  0x06);

    add_idt_entry((IDT_entry){(ptr)double_fault_handler, 8, idt_fault_flags, 0},
                  0x08);
    add_idt_entry((IDT_entry){(ptr)tss_fault_handler, 8, idt_fault_flags, 0},
                  0x0A);
    add_idt_entry((IDT_entry){(ptr)page_fault_handler, 8, idt_fault_flags, 0},
                  0x0E);

    add_idt_entry(
        (IDT_entry){(ptr)virtualization_fault_handler, 8, idt_fault_flags, 0},
        0x20);

    add_idt_entry(
        (IDT_entry){(ptr)division_fault_handler, 8, idt_fault_flags, 0}, 0);
}
