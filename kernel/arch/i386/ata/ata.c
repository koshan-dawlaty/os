#include "ata.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>

void init_ata()
{

    /* for (int i = PRIMARY_BUS_OFFSET; i < 10 + PRIMARY_BUS_OFFSET; i++) */
    /* { */
    /*     printf("%p\n", inb(i)); */
    /* } */

    /* uint8_t tmp = inb(PRIMARY_BUS_OFFSET + 6); */
    /* outb(PRIMARY_BUS_OFFSET + 6, tmp | (~(1 << 4))); */

    uint8_t value = inb(ATA_PRIMARY_STATUS);

    // wait for bit 7 to clear
    while (value & (1 << 7))
    {
        printf("value:%p\n", value);
        value = inb(ATA_PRIMARY_STATUS);
    }
    printf("test\n");

    outb(PRIMARY_BUS_OFFSET + 6, 0xA0);
    outb(PRIMARY_BUS_OFFSET + 2, 0);
    outb(PRIMARY_BUS_OFFSET + 3, 0);
    outb(PRIMARY_BUS_OFFSET + 4, 0);
    outb(PRIMARY_BUS_OFFSET + 5, 0);
    // IDENTIFY COMMAND
    outb(PRIMARY_BUS_OFFSET + 7, 0xEC);

    /* uint8_t inital_val = inb(PRIMARY_BUS_OFFSET + 7); */
    /* printf("%p\n", inital_val); */

    /* int is_floating = inital_val == 0xFF; */
    /* if (is_floating) */
    /* { */
    /*     // TODO: this should be a panic */
    /*     printf("could not find drive!\n"); */
    /*     return; */
    /* } */
}
