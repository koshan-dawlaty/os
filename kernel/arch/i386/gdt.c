#include "gdt.h"
#include <stdint.h>

typedef struct
{
    int limit;
    uint8_t access_byte;
    int flags;
    uint32_t base;
} GDT_entry;

struct GDT_register
{
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static uint64_t gdt[3] = {0};
static struct GDT_register gdtr;

uint64_t encode_gdt_entry(GDT_entry source)
{
    uint64_t output = 0;
    uint8_t *target = (uint8_t *)&output;
    // Check the limit to make sure that it can be encoded
    if (source.limit > 0xFFFFF)
    {
        // error
    }

    // Encode the limit
    target[0] = source.limit & 0xFF;
    target[1] = (source.limit >> 8) & 0xFF;
    target[6] = (source.limit >> 16) & 0x0F;

    // Encode the base
    target[2] = source.base & 0xFF;
    target[3] = (source.base >> 8) & 0xFF;
    target[4] = (source.base >> 16) & 0xFF;
    target[7] = (source.base >> 24) & 0xFF;

    // Encode the access byte
    target[5] = source.access_byte;

    // Encode the flags
    target[6] |= (source.flags << 4);

    return output;
}

void add_gdt_entry(GDT_entry entry, int index)
{
    gdt[index] = encode_gdt_entry(entry);
}

extern void reload_segments();

void load_gdt()
{
    asm volatile("lgdt (%0)" : : "r"(&gdtr));

    reload_segments();
}

void init_gdt()
{
    // null
    add_gdt_entry((GDT_entry){0, 0, 0, 0}, 0);

    // code
    add_gdt_entry((GDT_entry){0xFFFFF, 0x9A, 0xC, 0x0}, 1);
    // data
    add_gdt_entry((GDT_entry){0xFFFFF, 0x92, 0xC, 0x0}, 2);

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint32_t)&gdt;

    load_gdt();
}
