#include "pic.h"
#include "utils.h"

#include <stdint.h>

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2 + 1)

#define ICW1_ICW4 0x01      /* Indicates that ICW4 will be present */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01 /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02 /* Auto (normal) EOI */
#define ICW4_BUF_2 0x08
#define ICW4_BUF_1 0x0C
#define icw4_sfnm 0x10 /* special fully nested (not) */

#define PIC_EOI 0x20 /* End-of-interrupt command code */

void IRQ_set_mask(uint8_t IRQline)
{
    uint16_t port;
    uint8_t value;

    if (IRQline < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) | (1 << IRQline);
    outb(port, value);
}

void IRQ_clear_mask(uint8_t IRQline)
{
    uint16_t port;
    uint8_t value;

    if (IRQline < 8)
    {
        port = PIC1_DATA;
    }
    else
    {
        port = PIC2_DATA;
        IRQline -= 8;
    }
    value = inb(port) & ~(1 << IRQline);
    outb(port, value);
}

void remap_pic(int offset1, int offset2)
{
    uint8_t start_mask1, start_mask2;

    start_mask1 = inb(PIC1_DATA);
    start_mask2 = inb(PIC2_DATA);

    /* initialize command
     * makes pic wait for 3 bytes from data port
     * first bytes are vector offset
     */
    outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
    io_wait();

    // send vector offset
    outb(PIC1_DATA, offset1);
    io_wait();
    outb(PIC2_DATA, offset2);
    io_wait();

    // tells pic1 that pic2 is at irq2
    outb(PIC1_DATA, 4);
    io_wait();
    outb(PIC2_DATA, 2);
    io_wait();

    // tell the pics to use 8086 mode and not 8080 mode
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    // restore saved masks.
    outb(PIC1_DATA, start_mask1);
    outb(PIC2_DATA, start_mask2);
}

void PIC_sendEOI(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}
