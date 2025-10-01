#include "gdt.h"
#include "kernel/types.h"
#include <stdint.h>

typedef struct
{
    u32 limit;
    ptr base;
    u8 access;
    u8 flags;
} GDT_entry;

/* TODO: something similar to this is done in x86_64/idt.c and its bad */
typedef struct
{
    u64 one;
    u64 two;
} GDT_real;

GDT_real format_gdt_entry(GDT_entry entry)
{
    u64 one = 0;
    *(((u16 *)one)) = entry.limit;
    *(((u16 *)one) + 1) = entry.base;
    *(((u8 *)one) + 4) = (entry.base >> 16) & ((1 << 16) - 1);
    *(((u8 *)one) + 5) = entry.access;
    *(((u8 *)one) + 6) = entry.flags;
    *(((u8 *)one) + 6) &= (entry.flags << 4);
    *(((u8 *)one) + 7) = (entry.base >> 0x18) & ((1 << 8) - 1);
    u64 two = (entry.base >> 32) & ((1L << 32) - 1);

    return (GDT_real){one, two};
}

/* TODO: tss */
void init_gdt()
{
    /* i dont know if i need any gdt entries */
}

void set_kernel_stack(u32 stack)
{
}
