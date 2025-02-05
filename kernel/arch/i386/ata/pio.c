#include "ata.h"
#include "ata/ata_utils.h"
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

void read_ata_pio(uint8_t *buffer, uint16_t io_offset, uint16_t control_offset,
                  uint32_t sectors, uint32_t lba)
{

    wait_for_bit(io_offset, control_offset, 7, 0);

    ata_drive_select(io_offset, control_offset,
                     0xE0 | (0 << 4) | ((lba >> 24) & 0x0F));

    ata_send_command_24(io_offset, control_offset, sectors, lba, 0x20);

    for (size_t i = 0; i < sectors * 512; i += 2)
    {
        wait_for_bit(io_offset, control_offset, 7, 0);
        wait_for_bit(io_offset, control_offset, 3, 1);

        uint16_t data = inw(io_offset + ATA_DATA);

        buffer[i] = (uint8_t)(data) & 0xFF;
        buffer[i + 1] = (uint8_t)(data >> 8) & 0xFF;
    }
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

    for (size_t i = 0; i < sectors; i++)
    {
        for (size_t i = 0; i < 512 * sectors; i += 2)
        {

            uint16_t word = (buffer[i + 1] << 8) | (buffer[i]);

            outw(io_offset + ATA_DATA, word);
            io_wait();
        }

        buffer += 256;
        outb(io_offset + ATA_COMMAND, 0xE7);
        wait_for_bit(io_offset, control_offset, 7, 0);
    }
}
