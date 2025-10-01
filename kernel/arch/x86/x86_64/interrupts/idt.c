#include "interrupts/idt.h"
#include "interrupts/faults.h"
#include "interrupts/idt_internal.h"
#include "interrupts/pic.h"
#include "kernel/types.h"
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
    u64 one;
    u64 two;
} __attribute__((packed));

static struct idt_real idt[256] = {};

void add_idt_entry(IDT_entry entry, u8 index)
{
    u8 *ptr = ((u8 *)(idt)) + index * 16;

    ((u16 *)(ptr))[0] = (entry.offset & 0xFFFF);

    ((u16 *)(ptr))[1] = (entry.segment_selector);

    ptr[4] = (entry.stack_table);
    ptr[5] = (entry.flags);

    ((u16 *)(ptr))[3] = ((entry.offset >> 16) & 0xFFFF);
    ((u32 *)(ptr))[2] = ((entry.offset >> 32) & 0xFFFFFFFF);
    ((u32 *)(ptr))[3] = 0;
}

ISR(default_interrupt_handler, { printf("default interrupt handler\n"); });

extern void swap_segments();
void load_idt()
{

    asm volatile("lidt %0" : : "m"(idtr));

    /* swap_segments(); */

    remap_pic(0x20, 0x28);
    IRQ_clear_mask(0);
    /* IRQ_clear_mask(1); */

    asm volatile("sti");
}

ISR(pit_test, {
    printf("pit_test\n");
    PIC_sendEOI(0);
});

ISR(irq7, { printf("irq7!!!\n"); })

void init_idt()
{

    printf("%p\n", default_interrupt_handler);
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

    add_idt_entry((IDT_entry){.offset = (u64)pit_test,
                              .segment_selector = 8,
                              .flags = 0x8E,
                              .stack_table = 0},
                  0x20);

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
