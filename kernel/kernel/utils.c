#include "kernel/utils.h"
#include <stdint.h>

long align_long(long in, long align)
{

    if (in % align == 0)
    {
        return in;
    }

    return (in + align) - (in % align);
}

uint32_t bytes_to_32_bit_int(uint8_t one, uint8_t two, uint8_t three,
                             uint8_t four)
{
    return (one) | (two << 8) | (three << 16) | (four << 24);
}
