#include "gdt.h"
#include "kernel/types.h"
#include <stddef.h>
#include <stdio.h>

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
};

struct tss tss;

struct GDT_register
{
    u16 limit;
    u64 base;
} __attribute__((packed));

struct GDT_register gdtr;

u64 gdt[7] = {};

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

    gdt[5] = ((u64)limit & 0xFFFF) | ((base & 0xFFFFFF) << 16) |
             ((u64)0x89 << 40) | (((limit >> 16) & 0xF) << 48) |
             (((base >> 24) & 0xFF) << 56);
    gdt[6] = (base >> 32) & 0xFFFFFFFFULL;

    printf("%p\n", gdt[5]);
    printf("%p\n", gdt[6]);
    printf("%p\n", &tss);

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (ptr)&gdt;

    tss.ist1 = (ptr)(&double_fault_stack[sizeof(double_fault_stack)]);

    asm volatile("lgdt (%0)" : : "r"(&gdtr));
    swap_segments();

    flush_tss();
}

void tss_test()
{
    printf("TSS KERNEL STACK:%p\n", tss.rsp0);

    u64 *ptr = (u64 *)(tss.rsp0);
    for (size_t i = 0; i < 10; i++)
    {

        printf("%p ", *ptr);
        ptr++;
    }
}

void set_kernel_stack(ptr stack)
{
    tss.rsp0 = stack;
}
