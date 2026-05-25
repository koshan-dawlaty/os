#include "faults.h"
#include "idt_internal.h"
#include <kernel/stacktrace.h>

#include "interrupts/idt.h"
#include "kernel/types.h"
#include "lib/log.h"
#include <kernel/faults.h>

#include <stdint.h>
#include <stdio.h>

extern ptr get_cr2();

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

    log_multiline_start(&LOG_ERROR);
    loge("gpf!!!\n");
    loge("cs:%p\n", (void *)frame->cs);
    loge("error code:%p\n", (void *)frame->flags);
    loge("ip:%p\n", (void *)frame->ip);
    loge("sp:%p\n", (void *)frame->sp);
    loge("ss:%p", frame->ss);
    log_multiline_end(&LOG_ERROR);
    stacktrace_log();

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

    /* printf("page fault\n"); */
    void *addr = (void *)get_cr2();
    /* printf("addr:%p\n", addr); */
    /* printf("error code:%p\n", error_code); */
    /* printf("instruction pointer:%p\n", frame->ip); */

    bool not_present = (error_code & (1 << 0)) != 0;
    bool is_user = (error_code & (1 << 2)) != 0;
    bool instruction = (error_code & (1 << 4)) != 0;
    bool reserved = (error_code & (1 << 3)) != 0;

    page_fault(addr, (void *)frame->ip,
               (struct page_fault_flags){.not_present = not_present,
                                         .user = is_user,
                                         .instruction = instruction,
                                         .other = reserved});

    /* fault_handler_wait(); */
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
    const int num_bytes = 10;
    printf("%d bytes from %p (ip)\n", num_bytes, frame->ip);
    for (int i = 0; i < num_bytes; i++)
    {
        printf("%p ", *(u8 *)(frame->ip - 1 + i));
    }
    printf("\n");

    fault_handler_wait();
});

ISR(coprocesser_segment_overrun, {
    printf("coprocesser_segment_overrun\n");
    fault_handler_wait();
})

const u8 idt_fault_flags = 0x8F;

#ifdef CONFIG_x86_32
#define FAULT_IDT_ENTRY(function)                                              \
    (IDT_entry)                                                                \
    {                                                                          \
        (ptr) function, 8, idt_fault_flags                                     \
    }
#elif defined(CONFIG_x86_64)
#define FAULT_IDT_ENTRY(function)                                              \
    (IDT_entry){(ptr)function, 8, idt_fault_flags, 0}
#endif

void init_faults()
{

    for (uint8_t i = 0; i < 32; i++)
    {
        add_idt_entry(FAULT_IDT_ENTRY(default_fault_handler), i);
    }

    add_idt_entry(FAULT_IDT_ENTRY(gpf_handler), 0x0D);

    /* add_idt_entry((IDT_entry){(ptr)coprocesser_segment_overrun, 8,
     * idt_fault_flags, 0}, 9); */

    add_idt_entry(FAULT_IDT_ENTRY(invalid_opcode), 0x06);

    add_idt_entry(FAULT_IDT_ENTRY(double_fault_handler), 0x08);
    add_idt_entry(FAULT_IDT_ENTRY(tss_fault_handler), 0x0A);
    add_idt_entry(FAULT_IDT_ENTRY(page_fault_handler), 0x0E);

    add_idt_entry(FAULT_IDT_ENTRY(virtualization_fault_handler), 0x20);

    add_idt_entry(FAULT_IDT_ENTRY(division_fault_handler), 0);
}
