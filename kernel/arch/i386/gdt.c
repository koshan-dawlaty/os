#include "gdt.h"
#include <stdint.h>
#include <string.h>

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

typedef uint32_t esp;
typedef uint16_t ss;
typedef uint16_t padding;
typedef uint32_t ext_reg;
typedef uint16_t reg;

struct tss
{
    uint16_t link;
    padding padding0;
    esp esp0;
    ss ss0;
    padding padding1;
    esp esp1;
    ss ss1;
    padding padding2;
    esp esp2;
    ss ss2;
    padding padding3;
    ext_reg cr3;
    ext_reg eip;
    ext_reg eflags;
    ext_reg eax;
    ext_reg ecx;
    ext_reg edx;
    ext_reg ebx;
    ext_reg esp;
    ext_reg ebp;
    ext_reg esi;
    ext_reg edi;
    reg es;
    padding padding4;
    reg cs;
    padding padding5;
    reg ss;
    padding padding6;
    reg ds;
    padding padding7;
    reg fs;
    padding padding8;
    reg gs;
    padding padding9;
    reg ldtr;
    padding padding10;
    padding padding11;
    reg iopb;
    ext_reg ssp;
} __attribute__((packed));
struct tss tss;

static uint64_t gdt[6] = {0};
static struct GDT_register gdtr;

#define USER_STACK_SIZE 0x4000
volatile static uint8_t user_stack[USER_STACK_SIZE]
    __attribute__((aligned(16)));

volatile void *user_stack_top = &user_stack[USER_STACK_SIZE];

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
    target[6] = (source.limit >> 16) & 0xFF;

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
extern void flush_tss();

void load_gdt()
{

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base = (uint32_t)&gdt;

    asm volatile("lgdt (%0)" : : "r"(&gdtr));

    reload_segments();

    flush_tss();
}

void init_gdt()
{
    // null
    add_gdt_entry((GDT_entry){0, 0, 0, 0}, 0);

    // kernel
    // code
    add_gdt_entry((GDT_entry){0xFFFFF, 0x9A, 0xC, 0x0}, 1);
    // data
    add_gdt_entry((GDT_entry){0xFFFFF, 0x92, 0xC, 0x0}, 2);

    // user
    // code
    add_gdt_entry((GDT_entry){0xFFFFF, 0xFA, 0xC, 0x0}, 3);
    // data
    add_gdt_entry((GDT_entry){0xFFFFF, 0xF2, 0xC, 0x0}, 4);

    memset(&tss, 0, sizeof(tss));

    register int esp asm("esp");

    tss.ss0 = 0x10;
    tss.esp0 = esp;

    // tss
    add_gdt_entry((GDT_entry){sizeof(tss) - 1, 0x89, 0x0, (uint32_t)&tss}, 5);

    load_gdt();
}
