#ifndef UTILS_H
#define UTILS_H
#include <stdint.h>

long align_long(long in, long align);

uint32_t bytes_to_32_bit_int(uint8_t one, uint8_t two, uint8_t three,
                             uint8_t four);

#endif
