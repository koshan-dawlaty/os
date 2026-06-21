#include "gdt.h"
#include <stddef.h>
#include <stdio.h>
#include <types.h>

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

struct tss tss;

struct GDT_register
{
    u16 limit;
    u64 base;
} __attribute__((packed));

struct GDT_register gdtr;

u64 __attribute__((aligned(8))) gdt[7] = {};

extern void swap_segments();
extern void flush_tss();

extern ptr stack_top;

extern char double_fault_stack[4096];

/* TODO: tss */
void init_gdt()
{

    /* TODO: this is kinda stupid but i dont care */
    gdt[0] = 0;
    /* kernel */
    gdt[1] = 0x00AF9A000000FFFF; /* code */
    gdt[2] = 0x00AF92000000FFFF; /* data */

    /* user */
    gdt[3] = 0x00AFFA000000FFFF; /* code */
    gdt[4] = 0x00AFF2000000FFFF; /* data */

    tss.iopb = sizeof(struct tss);

    u64 base = (u64)&tss;
    u64 limit = sizeof(tss) - 1;
    tss.rsp0 = stack_top;

    gdt[5] = ((limit & 0xFFFF)) | ((base & 0xFFFFFF) << 16) |
             ((uint64_t)0x89 << 40) | // 0x89 = 64-bit TSS (available)
             ((limit >> 16) & 0xF) << 48 | ((base >> 24) & 0xFF) << 56;
    gdt[6] = (base >> 32) & 0xFFFFFFFFULL;

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (ptr)&gdt;

    /* tss.ist1 = (ptr)(&double_fault_stack[sizeof(double_fault_stack)]); */

    asm volatile("lgdt (%0)" : : "r"(&gdtr));
    swap_segments();

    flush_tss();
}

void set_kernel_stack(ptr stack)
{
    tss.rsp0 = stack;
}
