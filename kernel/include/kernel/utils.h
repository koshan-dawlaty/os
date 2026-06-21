#ifndef KERNEL_UTILS_H
#define KERNEL_UTILS_H
#include <scheduler.h>
#include <stdint.h>
#include <types.h>

#define MACRO_STR2(x) #x
#define MACRO_STR(x) MACRO_STR2(x)

long align_long(long in, long align);

static inline ptr_t ptr_align(ptr_t in, ptr_t align)
{
    return (in + align - 1) & ~(align - 1);
}

uint32_t bytes_to_32_bit_int(uint8_t one, uint8_t two, uint8_t three,
                             uint8_t four);
uint16_t bytes_to_16_bit_int(uint8_t one, uint8_t two);

static inline u32 word_from_buffer(u8 *buf)
{
    return *((u32 *)(buf));
}

static inline u16 short_from_buffer(u8 *buf)
{
    return *((u16 *)(buf));
}

static inline u64 long_from_buffer(u8 *buf)
{
    return *((u64 *)(buf));
}

void print_regsiters(struct registers regs);

#endif
