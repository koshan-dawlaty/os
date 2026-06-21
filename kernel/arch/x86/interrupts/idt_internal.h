#ifndef IDT_INTERNAL_H
#define IDT_INTERNAL_H
#include <stdint.h>
#include <types.h>

typedef struct
{

#ifdef CONFIG_x86_32
    uint32_t offset;
    uint16_t segment_selector;
    uint8_t flags;

#elif defined(CONFIG_x86_64)
    u64 offset;
    u16 segment_selector;
    u8 flags;
    u8 stack_table;
#endif

} IDT_entry;

void add_idt_entry(IDT_entry entry, uint8_t index);

#endif
