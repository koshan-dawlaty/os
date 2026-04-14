#ifndef GDT_H
#define GDT_H
#include "kernel/types.h"
#include <stdint.h>

struct tss
{
    u32 reserved;
    u64 rsp0;
    u64 rsp1;
    u64 rsp2;
    u64 reserved2;
    u64 ist1;
    u64 ist2;
    u64 ist3;
    u64 ist4;
    u64 ist5;
    u64 ist6;
    u64 ist7;
    u64 reserved3;
    u16 reserved4;
    u16 iopb;
} __attribute__((packed));

void gdt_init_tss(struct tss tss);

void init_gdt();
void set_kernel_stack(ptr stack);

#endif
