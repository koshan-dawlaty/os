#include "idt.h"
#include "keyboard_map.h"
#include "pic.h"

// TODO: fix include path
#include "../utils.h"

#include <stdint.h>
#include <stdio.h>

typedef struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

typedef struct
{
    uint32_t offset;
    uint16_t segment_selector;
    uint8_t flags;

} IDT_entry;

static idtr_t idtr;
__attribute__((aligned(0x10))) static uint64_t idt[256] = {0};

uint64_t format_IDT_entry(IDT_entry entry)
{
    uint64_t output = 0;
    uint8_t *ptr = (uint8_t *)&output;

    ptr[0] = (entry.offset) & 0xFF;
    ptr[1] = (entry.offset >> 8) & 0xFF;

    ptr[6] = (entry.offset >> 16) & 0xFF;
    ptr[7] = (entry.offset >> 24) & 0xFF;

    ptr[2] = (entry.segment_selector) & 0xFF;
    ptr[3] = (entry.segment_selector >> 8) & 0xFF;

    ptr[5] = (entry.flags);

    ptr[4] = 0;

    return output;
}

void add_idt_entry(IDT_entry entry, uint8_t index)
{
    idt[index] = format_IDT_entry(entry);
}

void load_idt()
{
    asm volatile("lidt %0" : : "m"(idtr));

    remap_pic(0x20, 0x28);
    IRQ_clear_mask(0);
    IRQ_clear_mask(1);

    asm volatile("sti");
}

struct interrupt_frame
{
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
};

ISR void gpf_handler(struct interrupt_frame *frame)
{
    printf("gpf!!!\n");
}

ISR void test_interrupt_handler(struct interrupt_frame *frame)
{

    printf("interrupt\n");
}

ISR void keyboard_interrupt_handler(struct interrupt_frame *frame)
{
    /* printf("keyboard isr\n"); */

    uint8_t scancode = inb(0x60);

    if (scancode < 0x81)
    {
        printf("%c", keyboard_map[scancode]);
        /* printf("%p\n", scancode); */
    }
    /* printf("%p\n", inb(0x60)); */

    PIC_sendEOI(1);
}

ISR void default_interrupt_handler(struct interrupt_frame *frame)
{
    printf("default interrupt\n");
}

// this is necessary for keyboard
ISR void pit_interrupt_handler(struct interrupt_frame *frame)
{
    /* printf("pit interrupt handler called\n"); */

    PIC_sendEOI(0);
}

void init_idt()
{

    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = sizeof(idt) - 1;

    for (int i = 32; i < 256; i++)
    {
        add_idt_entry((IDT_entry){0, 0, 0}, i);
        add_idt_entry((IDT_entry){(uint32_t)default_interrupt_handler, 8, 0x8E},
                      i);
    }
    add_idt_entry((IDT_entry){(uint32_t)test_interrupt_handler, 0x8, 0x8E},
                  0x50);
    add_idt_entry((IDT_entry){(uint32_t)keyboard_interrupt_handler, 0x8, 0x8E},
                  0x21);

    add_idt_entry((IDT_entry){(uint32_t)pit_interrupt_handler, 0x8, 0x8E},
                  0x20);

    add_idt_entry((IDT_entry){(uint32_t)gpf_handler, 8, 0x8F}, 0x0D);
    add_idt_entry((IDT_entry){(uint32_t)gpf_handler, 8, 0x8F}, 0x08);

    load_idt();

    /* outb(0x60, 0xFF); */
    /* printf("%p\n", inb(0x60)); */
    /* printf("%p\n", inb(0x60)); */

    /* *(char *)(0x1) = 'a'; */

    printf("calling interrupt\n");
    asm volatile("int $0x50");
    /* asm volatile("int $0x21"); */
    printf("called interrupt\n");
}
