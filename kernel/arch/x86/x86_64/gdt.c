#include "gdt.h"
#include "kernel/types.h"
#include <stddef.h>
#include <stdio.h>

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

void gdt_init_tss(struct tss t)
{

    /* TODO: this is kinda stupid but i dont care */
    gdt[0] = 0;
    /* kernel */
    gdt[1] = 0x00AF9A000000FFFF; /* code */
    gdt[2] = 0x00AF92000000FFFF; /* data */

    /* user */
    gdt[3] = 0x00AFFA000000FFFF; /* code */
    gdt[4] = 0x00AFF2000000FFFF; /* data */

    t.iopb = sizeof(struct tss);

    u64 base = (u64)&t;
    u64 limit = sizeof(t) - 1;
    t.rsp0 = stack_top;

    gdt[5] = ((limit & 0xFFFF)) | ((base & 0xFFFFFF) << 16) |
             ((u64)0x89 << 40) | ((limit >> 16) & 0xF) << 48 |
             ((base >> 24) & 0xFF) << 56;
    gdt[6] = (base >> 32) & 0xFFFFFFFFULL;

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (ptr)&gdt;

    /* tss.ist1 = (ptr)(&double_fault_stack[sizeof(double_fault_stack)]); */

    asm volatile("lgdt (%0)" : : "r"(&gdtr));
    swap_segments();

    flush_tss();
}

void init_gdt()
{
    gdt_init_tss(tss);
}

/* from smp.c */
extern struct tss *apic_tss;
extern u8 get_cpu_id();

void set_kernel_stack(ptr stack)
{
    apic_tss[get_cpu_id()].rsp0 = stack;
    /* tss.rsp0 = stack; */
}
