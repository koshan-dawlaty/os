#include "kernel/utils.h"
#include "scheduler.h"
#include <stdint.h>
#include <stdio.h>

long align_long(long in, long align)
{

    if (in % align == 0)
    {
        return in;
    }

    /* round down instead of up */
    in -= align;

    return (in + align) - (in % align);
}

uint32_t bytes_to_32_bit_int(uint8_t one, uint8_t two, uint8_t three,
                             uint8_t four)
{
    return (one) | (two << 8) | (three << 16) | (four << 24);
}

uint16_t bytes_to_16_bit_int(uint8_t one, uint8_t two)
{
    return (one) | (two << 8);
}

void print_regsiters(struct registers regs)
{
    printf("eax:%p, ebx:%p, ecx:%p, edx:%p\nesi:%p, edi:%p, ebp:%p, esp:%p\n",
           regs.eax, regs.ebx, regs.ecx, regs.edx, regs.esi, regs.edi, regs.ebp,
           regs.esp);
}
