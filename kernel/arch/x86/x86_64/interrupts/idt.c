#include "interrupts/idt.h"
#include "interrupts/faults.h"
#include "interrupts/idt_internal.h"
#include "interrupts/pic.h"
#include "kernel/types.h"
#include "keyboard/x86_keyboard.h"
#include "pit/pit.h"
#include "sys/types.h"
#include <stddef.h>
#include <stdio.h>

typedef struct
{
    u16 size;
    u64 addr;
} __attribute__((packed)) idtr_t;

static idtr_t idtr;

/* should just be a 128 bit value */
struct idt_real
{
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 type_attr;
    u16 offset_mid;
    u32 offset_high;
    u32 zero;
} __attribute__((packed));

static struct idt_real idt[256] = {};

void add_idt_entry(IDT_entry entry, u8 index)
{
    struct idt_real *real = &idt[index];
    real->offset_low = entry.offset & 0xFFFF;
    real->selector = entry.segment_selector;
    real->ist = entry.stack_table;
    real->type_attr = entry.flags;
    real->offset_mid = (entry.offset >> 16) & 0xFFFF;
    real->offset_high = (entry.offset >> 32) & 0xFFFFFFFF;
    real->zero = 0;
}

ISR(default_interrupt_handler, { printf("default interrupt handler\n"); });

extern void swap_segments();
void load_idt()
{

    asm("lidt %0" : : "m"(idtr));

    remap_pic(0x20, 0x28);
    IRQ_clear_mask(0);
    IRQ_clear_mask(1);

    asm("sti");
}

ISR(pit_test, {
    printf("pit_test\n");
    PIC_sendEOI(0);
});

ISR(irq7, { printf("irq7!!!\n"); })

extern void pit_test_asm();

char double_fault_stack[4096] = {};
void init_idt()
{

    /* printf("%p\n", default_interrupt_handler); */
    idtr.addr = (u64)&idt[0];
    idtr.size = sizeof(idt) - 1;

    /* printf("%d\n", 12345); */

    for (size_t i = 0; i < 256; i++)
    {
        add_idt_entry((IDT_entry){.offset = (u64)default_interrupt_handler,
                                  .segment_selector = 8,
                                  .flags = 0x8E,
                                  .stack_table = 0},
                      i);
    }

    init_faults();

    add_idt_entry((IDT_entry){.offset = (u64)pit_handler,
                              .segment_selector = 8,
                              .flags = 0x8E,
                              .stack_table = 0},
                  0x20);

    add_idt_entry((IDT_entry){.offset = (u64)keyboard_interrupt_handler,
                              .segment_selector = 8,
                              .flags = 0x8E,
                              .stack_table = 0},
                  0x21);

    /* add_idt_entry((IDT_entry){.offset = (u64)irq7, */
    /*                           .segment_selector = 8, */
    /*                           .flags = 0x8E, */
    /*                           .stack_table = 0}, */
    /*               0x27); */

    /* add_idt_entry((IDT_entry){.offset = (u64)interrupt_test,
                                 .segment_selector = 8,
                                 .flags = 0x8E,
                                 .stack_table = 0},
                     0x50);
    */
    load_idt();
}
