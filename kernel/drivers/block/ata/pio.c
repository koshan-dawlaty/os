#include "ata/ata.h"
#include "ata_utils.h"
#include "interrupts/idt.h"
#include "interrupts/pic.h"
#include "stdlib.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

void print_all_regs(uint16_t offset)
{
    printf("data: %p\n", inb(offset + ATA_DATA));
    printf("error: %p\n", inb(offset + ATA_ERROR));
    printf("features: %p\n", inb(offset + ATA_FEATURES));
    printf("sector count: %p\n", inb(offset + ATA_SECTOR_COUNT));
    printf("lba low: %p\n", inb(offset + ATA_LBA_LO));
    printf("lba mid: %p\n", inb(offset + ATA_LBA_MID));
    printf("lba high: %p\n", inb(offset + ATA_LBA_HI));
    printf("drive head: %p\n", inb(offset + ATA_DRIVE_HEAD));
    printf("status: %p\n", inb(offset + ATA_STATUS));
}

ISR(ata_isr_1, {
    printf("ata isr 1\n");

    PIC_sendEOI(14);
})

ISR(ata_isr_2, {
    printf("ata isr 2\n");
    PIC_sendEOI(15);
})

void read_ata_pio(uint8_t *buffer, uint16_t io_offset, uint16_t control_offset,
                  uint32_t sectors, uint32_t lba)
{

    /* wait_for_bit(io_offset, control_offset, 7, 0); */

    outb(io_offset + 1, 0);

    /* ata_drive_select(io_offset, control_offset, */
    /*                  0xE0 | (0 << 4) | ((lba >> 24) & 0x0F)); */

    ata_send_command_24(io_offset, control_offset, sectors, lba, 0x20);

    /* printf("status:%p, error:%p\n", inb(io_offset + 7), inb(io_offset + 1));
     */

    /* printf("starting read\n"); */

    /* printf("GOING TO READ %d sectors\n", sectors); */
    for (size_t s = 0; s < sectors; s++)
    {
        wait_for_bit(io_offset, control_offset, 7, 0);
        wait_for_bit(io_offset, control_offset, 3, 1);

        for (size_t i = 0; i < 256; i++)
        {

            uint16_t data = inw(io_offset + ATA_DATA);
            /* printf("%p", data); */

            buffer[s * 512 + i * 2] = (uint8_t)(data) & 0xFF;
            buffer[s * 512 + i * 2 + 1] = (uint8_t)(data >> 8);

            /* if (i < 50 && s < 1) */
            /* { */
            /*     printf("%p ", data); */
            /* } */
        }
        /* printf("sector %d done\n", s); */
    }
    /* printf("\n"); */

    /* wait_for_bit(io_offset, control_offset, 7, 0); */
    /* wait_for_bit(io_offset, control_offset, 3, 0); */
    /* printf("waiting\n"); */
    ata_wait_for_mask(io_offset, 7 | 3, 0);
    /* printf("done\n"); */
}

/*
 * buffer_size is in bytes
 */
void write_ata_pio(uint16_t io_offset, uint16_t control_offset, uint32_t lba,
                   uint8_t *buffer, size_t buffer_size)
{

    uint32_t sectors = buffer_size / 512;

    wait_for_bit(io_offset, control_offset, 7, 0);

    ata_drive_select(io_offset, control_offset,
                     0xE0 | (0 << 4) | ((lba >> 24) & 0x0F));

    ata_send_command_24(io_offset, control_offset, sectors, lba, 0x30);

    for (size_t j = 0; j < sectors; j++)
    {
        wait_for_bit(io_offset, control_offset, 7, 0);
        wait_for_bit(io_offset, control_offset, 3, 1);

        for (size_t i = j * 512; i < (j + 1) * 512; i += 2)
        {

            uint16_t word = (buffer[i + 1] << 8) | (buffer[i]);

            outw(io_offset + ATA_DATA, word);
        }

        /* outb(io_offset + ATA_COMMAND, 0xE7); */
    }

    outb(io_offset + ATA_COMMAND, 0xE7);
}
