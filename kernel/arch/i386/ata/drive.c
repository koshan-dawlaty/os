#include "ata/ata.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// TODO: may hang if drive is not present

/*
 * returns 0 if succeeded
 *
 * returns 1 if drive is not ata / not a harddisk
 *
 * info_buf will be ignored if null
 *
 * info_buf should hold 256 uint16_t values (512 bytes)
 */
int send_identify(uint16_t io_offset, uint16_t control_offset,
                  uint16_t *info_buf)
{

    /* printf("%p\n", inb(io_offset + ATA_STATUS)); */
    /* outb(control_offset + ATA_CONTROL, 0x04); // Soft reset command (SRST) */
    /* io_wait(); */
    /* outb(control_offset + ATA_CONTROL, 0x00); // Clear SRST */
    /* io_wait(); */

    for (int i = 0; i < 10000; i++)
    {
        io_wait();
    }

    /* printf("%p\n", inb(io_offset + ATA_STATUS)); */

    ata_drive_select(io_offset, control_offset, 0xA0);

    outb(io_offset + ATA_FEATURES, 0);

    outb(io_offset + ATA_SECTOR_COUNT, 0);
    outb(io_offset + ATA_LBA_LO, 0);
    outb(io_offset + ATA_LBA_MID, 0);
    outb(io_offset + ATA_LBA_HI, 0);

    // IDENTIFY command
    outb(io_offset + ATA_COMMAND, 0xEC);

    uint8_t status;
    do
    {
        status = inb(control_offset + ATA_ALT_STATUS);
        /* status = inb(io_offset + ATA_STATUS); */

        if (inb(io_offset + ATA_LBA_MID) != 0 ||
            inb(io_offset + ATA_LBA_HI) != 0)
        {
            printf("not ata!!\n");
            return 1;
        }
    } while (status & (1 << 7) && !(status & (1 << 3)));

    for (uint8_t i = 0; i < 255; i++)
    {

        uint16_t data = inb(io_offset + ATA_DATA);
        /* printf("%p", data); */

        if (i == 0 && data == 0)
        {
            return 1;
        }

        // this is kinda slow, hopefully gcc will create 2 loops as an
        // optimization
        if (info_buf != NULL)
        {
            info_buf[i] = data;
        }
    }

    // no idea why, if this is not here, reading from the disk will always be
    // zero
    for (int i = 0; i < 256; i++)
    {
        inw(io_offset + ATA_DATA);
    }

    return 0;
}

ata_drive *get_ata_drive(uint8_t drive_num)
{
    ata_drive *drive = malloc(sizeof(ata_drive));

    if ((drive_num) % 2 == 0)
    {
        drive->is_drive1 = 1;
    }
    else
    {
        drive->is_drive1 = 0;
    }

    if (drive_num < 2)
    {
        drive->is_primary = 1;
        // technically this should be determined from pci
        drive->io_offset = ATA_PRIMARY_IO;
        drive->control_offset = ATA_PRIMARY_CONTROL;
    }
    else
    {
        drive->is_primary = 0;
        drive->io_offset = ATA_SECONDARY_IO;
        drive->control_offset = ATA_SECONDARY_CONTROL;
    }

    uint16_t info_buf[256];

    if (send_identify(drive->io_offset, drive->control_offset, info_buf))
    {
        // error
        return NULL;
    }

    if (info_buf[83] & (1 << 10))
    {
        drive->is_lba48 = 1;
    }
    else
    {
        drive->is_lba48 = 0;
    }

    if (info_buf[63] & 1 || info_buf[63] & (1 << 1) || info_buf[63] & (1 << 2))
    {

        drive->is_dma = 1;
    }
    else
    {
        drive->is_dma = 0;
    }
    /* printf("%p\n", info_buf[63]); */

    /* if (info_buf[49] & (1 << 8)) */
    /* { */
    /*     drive->is_dma = 1; */
    /* } */
    /* else */
    /* { */
    /*     drive->is_dma = 0; */
    /* } */

    return drive;
}

void read_drive(ata_drive *drive, uint8_t *buffer, uint32_t amount,
                uint64_t lba)
{

    /* if (!drive->is_dma) */
    /* { */
    read_ata_pio(buffer, drive->io_offset, drive->control_offset, amount / 512,
                 lba);
    /* } */
}

void write_drive(ata_drive *drive, uint8_t *buffer, uint32_t buffer_size,
                 uint64_t lba)
{

    /* if (!drive->is_dma) */
    /* { */
    write_ata_pio(drive->io_offset, drive->control_offset, lba, buffer,
                  buffer_size);
    /* } */
}
