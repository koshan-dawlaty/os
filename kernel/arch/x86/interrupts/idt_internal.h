#ifndef IDT_INTERNAL_H
#define IDT_INTERNAL_H
#include <stdint.h>

typedef struct
{
    uint32_t offset;
    uint16_t segment_selector;
    uint8_t flags;

} IDT_entry;

void add_idt_entry(IDT_entry entry, uint8_t index);

#endif
