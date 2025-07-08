#include "idt.h"
#include "faults.h"
#include "idt_internal.h"
#include "pic.h"
#include "pit/pit.h"

#include "ata/ata.h"

#include "kernel/keyboard.h"
#include "keyboard/x86_keyboard.h"
#include "utils.h"

extern ISR_ATTRIBUTE void syscall_handler_asm(struct interrupt_frame *frame);

#include <stdint.h>
#include <stdio.h>

typedef struct
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idtr_t;

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
    IRQ_clear_mask(2);

    asm volatile("sti");
}

ISR(default_interrupt_handler, { printf("default interrupt\n"); })

void init_idt()
{
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = sizeof(idt) - 1;

    for (int i = 32; i < 255; i++)
    {
        add_idt_entry(
            (IDT_entry){
                (uint32_t)default_interrupt_handler,
                8,
                0x8E,
            },
            i);
    }

    init_faults();

    add_idt_entry((IDT_entry){(uint32_t)keyboard_interrupt_handler, 0x8, 0x8E},
                  0x21);
    add_idt_entry((IDT_entry){(uint32_t)ata_isr_1, 0x8, 0x8E}, 14 + 0x20);
    add_idt_entry((IDT_entry){(uint32_t)ata_isr_2, 0x8, 0x8E}, 15 + 0x20);

    /* add_idt_entry((IDT_entry){(uint32_t)pit_handler, 0x8, 0x8E}, 0x20); */
    add_idt_entry((IDT_entry){(uint32_t)pit_handler_asm, 0x8, 0x8E}, 0x20);

    // can use syscall instruction instead in long mode
    add_idt_entry((IDT_entry){(uint32_t)syscall_handler_asm, 0x8, 0xEE}, 0x80);

    load_idt();
    keyboard_init();
    /* outb(0x60, 0xFF); */
    /* printf("%x\n", inb(0x60)); */
    /* printf("%x\n", inb(0x60)); */

    /* *(char *)(0x1) = 'a'; */

    /* printf("calling interrupt\n"); */
    /* asm volatile("int $0x50"); */
    /* printf("called interrupt\n"); */
}
