#include "utils.h"
#include "ata/ata.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
// could maybe be in utils
void ata_drive_select(uint16_t io_offset, uint16_t control_offset,
                      uint8_t drive)
{
    outb(io_offset + ATA_DRIVE_HEAD, drive);

    for (int i = 0; i < 14; i++)
    {
        inb(control_offset + ATA_ALT_STATUS);
    }
    uint8_t result = inb(control_offset + ATA_ALT_STATUS);

    if ((result & (1 << 7)))
    {
        // drive still has status busy
        printf("drive still busy\n");
    }
}

void wait_for_bit(uint16_t io_offset, uint16_t control_offset, uint8_t bit,
                  uint8_t set)
{
    uint8_t status;
    while (1)
    {
        status = inb(io_offset + ATA_STATUS);

        if (set)
        {
            if (status & (1 << bit))
            {
                break;
            }
        }
        else
        {
            if (!(status & (1 << bit)))
            {
                break;
            }
        }
    }
}

void ata_send_command_24(uint16_t io_offset, uint16_t control_offset,
                         uint8_t sectors, uint32_t lba, uint8_t command)
{

    outb(io_offset + ATA_ERROR, 0);
    outb(io_offset + ATA_SECTOR_COUNT, (unsigned char)sectors);
    outb(io_offset + ATA_LBA_LO, (unsigned char)lba);
    outb(io_offset + ATA_LBA_MID, (unsigned char)lba >> 8);
    outb(io_offset + ATA_LBA_HI, (unsigned char)lba >> 16);

    wait_for_bit(io_offset, control_offset, 7, 0);
    wait_for_bit(io_offset, control_offset, 6, 1);

    outb(io_offset + ATA_COMMAND, command);

    wait_for_bit(io_offset, control_offset, 7, 0);
    wait_for_bit(io_offset, control_offset, 3, 1);
}
