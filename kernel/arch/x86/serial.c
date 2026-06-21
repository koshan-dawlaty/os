#include <interrupts/pic.h>
#include <kernel/interrupts.h>
#include <serial.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <types.h>
#include <utils.h>
#include <wait_queue.h>

void serial_writestring(const char *str, size_t len, u16 port)
{
    for (size_t i = 0; i < len; i++)
    {
        serial_write(port, str[i]);
    }
}

WAIT_QUEUE_DEFINE(com1_queue);

ISR(com1_interrupt_handler, {
    wait_queue_done(&com1_queue);
    PIC_sendEOI(4);
});

void serial_init(uint16_t port)
{
    IRQ_clear_mask(4);
    register_interrupt_handler(0x24, com1_interrupt_handler,
                               (int_handler_flags){.is_user = false});
    outb(port + 1, 0x01);
    outb(port + 3, 0x80); // Enable DLAB (set baud rate divisor)
    outb(port + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
    outb(port + 1, 0x00); //                  (hi byte)
    outb(port + 3, 0x03); // 8 bits, no parity, one stop bit
    /* outb(port + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
     */
    outb(port + 2, 0x07); // Enable FIFO, clear them, with 1-byte threshold
    outb(port + 4, 0x0B); // IRQs enabled, RTS/DSR set
    outb(port + 4, 0x1E); // Set in loopback mode, test the serial chip
    outb(port + 0, 0xAE); // Test serial chip (send byte 0xAE and check if
                          // serial returns same byte)

    /* outb(port + 1, (1 << 0)); /1* enable interrupts *1/ */

    if (inb(port) != 0xAE)
    {
        printf("error when initalizing serial\n");
        return;
    }

    outb(port + 4, 0x0F);
}

bool is_transmit_empty(uint16_t port)
{
    return inb(port + 5) & 0x20;
}

void serial_write(uint16_t port, uint8_t data)
{
    while (!is_transmit_empty(port))
    {
    }

    outb(port, data);
}

int serial_recived(u16 port)
{
    return inb(port + 5) & 1;
}

u8 serial_read(u16 port)
{
    wait_queue_wait(&com1_queue);

    return inb(port);
}
