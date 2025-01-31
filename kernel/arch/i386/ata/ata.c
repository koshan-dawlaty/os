#include "ata.h"
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

/*
 * waits for bit to be set if set is true, otherwise waits for it to clear
 *
 * bit is from 0-7
 */
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

/*
 * reads status register 15 times to correctly select a drive
 *
 * very slow
 */
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

ISR(ata_isr_1, {
    /* printf("ata isr 1\n"); */

    PIC_sendEOI(14);
})

ISR(ata_isr_2, {
    /* printf("ata isr 2\n"); */
    PIC_sendEOI(15);
})

/*
 * buffer should hold (256 * sectors) 16 bit values ((512 * sectors) bytes)
 */
void read_ata_pio(uint16_t *buffer, uint16_t io_offset, uint16_t control_offset,
                  uint32_t sectors, uint32_t lba)
{

    wait_for_bit(io_offset, control_offset, 7, 0);

    ata_drive_select(io_offset, control_offset,
                     0xE0 | (0 << 4) | ((lba >> 24) & 0x0F));

    outb(io_offset + ATA_ERROR, 0);
    outb(io_offset + ATA_SECTOR_COUNT, (unsigned char)sectors);
    outb(io_offset + ATA_LBA_LO, (unsigned char)lba);
    outb(io_offset + ATA_LBA_MID, (unsigned char)lba >> 8);
    outb(io_offset + ATA_LBA_HI, (unsigned char)lba >> 16);

    wait_for_bit(io_offset, control_offset, 7, 0);
    wait_for_bit(io_offset, control_offset, 6, 1);

    // READ SECTORS
    outb(io_offset + ATA_COMMAND, 0x20);

    wait_for_bit(io_offset, control_offset, 7, 0);
    wait_for_bit(io_offset, control_offset, 3, 1);

    for (size_t i = 0; i < 256 * sectors; i++)
    {
        wait_for_bit(io_offset, control_offset, 7, 0);
        wait_for_bit(io_offset, control_offset, 3, 1);

        uint16_t data = inw(io_offset + ATA_DATA);

        buffer[i] = data;
    }
}

/*
 * buffer_size is in bytes
 */
void write_ata_pio(uint16_t io_offset, uint16_t control_offset, uint32_t lba,
                   uint16_t *buffer, size_t buffer_size)
{

    uint32_t sectors = buffer_size / 512;

    wait_for_bit(io_offset, control_offset, 7, 0);

    ata_drive_select(io_offset, control_offset,
                     0xE0 | (0 << 4) | ((lba >> 24) & 0x0F));

    outb(io_offset + ATA_ERROR, 0);
    outb(io_offset + ATA_SECTOR_COUNT, (unsigned char)sectors);
    outb(io_offset + ATA_LBA_LO, (unsigned char)lba);
    outb(io_offset + ATA_LBA_MID, (unsigned char)lba >> 8);
    outb(io_offset + ATA_LBA_HI, (unsigned char)lba >> 16);

    wait_for_bit(io_offset, control_offset, 7, 0);
    wait_for_bit(io_offset, control_offset, 6, 1);

    outb(io_offset + ATA_COMMAND, 0x30);

    io_wait();

    wait_for_bit(io_offset, control_offset, 7, 0);
    wait_for_bit(io_offset, control_offset, 3, 1);

    for (size_t i = 0; i < sectors; i++)
    {
        for (size_t i = 0; i < 256 * sectors; i++)
        {

            outw(io_offset + ATA_DATA, buffer[i]);
            io_wait();
        }

        buffer += 256;
        outb(io_offset + ATA_COMMAND, 0xE7);
        wait_for_bit(io_offset, control_offset, 7, 0);
    }
}

void init_ata()
{

    IRQ_clear_mask(14);
    IRQ_clear_mask(15);

    if (!send_identify(ATA_PRIMARY, NULL))
    {
        /* printf("identify succeeded!\n"); */
    }
    else
    {
        printf("ata identify failed\n");
        return;
    }
}
