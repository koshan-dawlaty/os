#include "interrupts/pic.h"
#include "kernel/interrupts.h"
#include "kernel/keyboard.h"
#include "userspace/mouse.h"
#include <interrupts/idt.h>
#include <lib/log.h>
#include <panic.h>
#include <stdbool.h>
#include <stdio.h>
#include <types.h>
#include <utils.h>

#define PS2_COMMAND 0x64
#define PS2_STATUS 0x64
#define PS2_RESPONSE 0x60
#define PS2_DATA 0x60

#define PS2_TEST_PASSED 0x55
#define PS2_TEST_FAILED 0xFC

#define PS2_TIMEOUT 100

/* returns -1 on error, 0 on success */
int ps2_write_dev(u8 byte, bool is_port2)
{
    if (is_port2)
    {
        outb(PS2_COMMAND, 0xD4);
    }

    size_t count = 0;

    /* write to first device */
    u8 status;
    do
    {
        status = inb(PS2_STATUS);
        count++;

        if (count > PS2_TIMEOUT)
        {
            return -1;
        }
    } while ((status & (1 << 1)));

    outb(PS2_DATA, byte);

    return 0;
}

u8 ps2_read_poll(bool is_port2)
{
    u8 status;
    do
    {
        status = inb(PS2_STATUS);
        /* printf("%p\n", status); */
    } while (!(status & (1 << 0)));

    return inb(PS2_DATA);
}

/*
 * returns true if the test was successful
 */
static bool ps2_self_test()
{

    outb(PS2_COMMAND, 0xAA);
    u8 response = inb(PS2_RESPONSE);

    if (response != PS2_TEST_PASSED)
    {
        /* TODO: maybe panic here? or dont print anything because this returns a
         * value */
        if (response != PS2_TEST_FAILED)
        {
            printf("ps2 test self test failed and returned unknown value:0x%p",
                   response);
        }
        else
        {
            printf("ps2 self test failed\n");
        }
        return false;
    }

    return true;
}

static bool ps2_has_two_ports()
{
    outb(PS2_COMMAND, 0xA8);
    outb(PS2_COMMAND, 0x20);
    u8 config = inb(PS2_RESPONSE);
    bool retval = !(config & (1 << 5));
    outb(PS2_COMMAND, 0xA7);

    return retval;
}

#define PS2_TEST_FIRST 0xAB
#define PS2_TEST_SECOND 0xA9
/*
 * should use either PS2_TEST_FIRST or PS2_TEST_SECOND as the argument
 * returns true if the test succeeded
 */
static bool ps2_test_port(u8 port_test)
{
    outb(PS2_COMMAND, port_test);
    return inb(PS2_RESPONSE) == 0;
}

static u8 ps2_identify_dev(bool is_port2)
{
    ps2_write_dev(0xF5, is_port2);
    if (ps2_read_poll(is_port2) != 0xFA)
    {
        printf("error\n");
    }

    ps2_write_dev(0xF2, is_port2);

    if (ps2_read_poll(is_port2) != 0xFA)
    {
        printf("error\n");
    }

    u8 id = ps2_read_poll(is_port2);

    ps2_write_dev(0xF4, is_port2);
    if (ps2_read_poll(is_port2) != 0xFA)
    {
        printf("error\n");
    }

    return id;
}

int ps2_init()
{
    /* disable both ports */
    outb(PS2_COMMAND, 0xAD);
    outb(PS2_COMMAND, 0xA7);

    /* flush output buffer */
    inb(PS2_RESPONSE);

    /* TODO: set controller configuration? */

    ps2_self_test();

    /* check for second port */
    bool has_two_ports = ps2_has_two_ports();

    if (!ps2_test_port(PS2_TEST_FIRST))
    {
        panic("ps2 test for port 1 failed");
    }

    if (has_two_ports)
    {
        if (!ps2_test_port(PS2_TEST_SECOND))
        {
            panic("ps2 test for port 2 failed");
        }
    }

    /* enable interrupts for device 1 */
    outb(PS2_COMMAND, 0x20);
    u8 command = inb(PS2_RESPONSE);
    command &= ~(1 | (1 << 1));
    outb(PS2_COMMAND, 0x60);
    outb(PS2_DATA, command);

    ps2_identify_dev(false);
    ps2_identify_dev(true);

    /* outb(PS2_COMMAND, 0xAE); */

    /* ps2_write_dev(0xA8, false); */
    /* printf("%p\n", ps2_read_poll(false)); */

    outb(PS2_COMMAND, 0x20);
    u8 command2 = inb(PS2_RESPONSE);
    command2 |= (1 | (1 << 1));
    outb(PS2_COMMAND, 0x60);
    outb(PS2_DATA, command2);

    return 0;
}

void ps2_interrupt()
{
    u8 status = inb(0x64);
    if (status & 1)
    {
        if (status & (1 << 5))
        {
            /* mouse */
            mouse_ps2_interrupt();
        }
        else
        {
            keyboard_ps2_interrupt();
        }
    }
}

INT_HANDLER(ps2_int_handler_1, {
    ps2_interrupt();
    PIC_sendEOI(1);
})

INT_HANDLER(ps2_int_handler_12, {
    ps2_interrupt();
    PIC_sendEOI(12);
})
