#ifndef PIC_H
#define PIC_H
#include <stdint.h>

void remap_pic(int offset1, int offset2);
void IRQ_clear_mask(uint8_t IRQline);
void IRQ_set_mask(uint8_t IRQline);
void PIC_sendEOI(uint8_t irq);

#endif
