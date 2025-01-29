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

    printf("%p\n", inb(io_offset + ATA_STATUS));

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
    printf("ata isr 1\n");

    PIC_sendEOI(14);
})

ISR(ata_isr_2, {
    printf("ata isr 2\n");
    PIC_sendEOI(15);
})

void wait_for_data(uint16_t io_offset, uint16_t control_offset)
{
    uint8_t status;
    while (1)
    {
        status = inb(io_offset + ATA_STATUS);
        /* printf("%p\n", status); */
        io_wait();
        io_wait();
        io_wait();
        io_wait();
        io_wait();

        if (status & (1 << 7))
        {
            continue;
        }

        if (status & (1 << 0) || status & (1 << 5))
        {
            printf("error\n");

            return;
        }

        if (status & (1 << 3))
        {
            break;
        }
    }
}

/*
 * buffer should hold 256 * sectors 16 bit values (512 * sectors bytes)
 */
void read_ata(uint16_t *buffer, uint16_t io_offset, uint16_t control_offset,
              uint32_t sectors, uint32_t lba)
{
    /* print_all_regs(ATA_PRIMARY_IO); */
    /* printf("%p\n", inb(io_offset + ATA_STATUS) & (1 << 7)); */

    uint8_t status;
    for (;;)
    {
        status = inb(io_offset + ATA_STATUS);
        if (!(status & (1 << 7)))
        { // BSY cleared
            break;
        }

        io_wait();
        io_wait();
        io_wait();
        io_wait();
        io_wait();

        /* printf("%p\n", status); */
    }

    /* outb(io_offset + ATA_DRIVE_HEAD, */
    /*      0xE0 | ((lba >> 24) & 0x0F)); // Select drive, set high bits of LBA
     */

    ata_drive_select(io_offset, control_offset,
                     0xE0 | (0 << 4) | ((lba >> 24) & 0x0F));

    outb(io_offset + ATA_ERROR, 0);
    outb(io_offset + ATA_SECTOR_COUNT, (unsigned char)sectors);
    outb(io_offset + ATA_LBA_LO, (unsigned char)lba);
    outb(io_offset + ATA_LBA_MID, (unsigned char)lba >> 8);
    outb(io_offset + ATA_LBA_HI, (unsigned char)lba >> 16);

    /* print_all_regs(io_offset); */
    do
    {
        status = inb(io_offset + ATA_STATUS);
    } while ((status & (1 << 7)) || !(status & (1 << 6)));

    // READ SECTORS
    outb(io_offset + ATA_COMMAND, 0x20);

    /* print_all_regs(ATA_PRIMARY_IO); */

    wait_for_data(io_offset, control_offset);

    for (size_t i = 0; i < 256 * sectors; i++)
    {
        uint16_t data = inw(io_offset + ATA_DATA);

        buffer[i] = data;
    }

    /* printf("test\n"); */
}

void init_ata()
{

    IRQ_clear_mask(14);
    IRQ_clear_mask(15);

    if (!send_identify(ATA_PRIMARY, NULL))
    {
        printf("identify succeeded!\n");
    }
    else
    {
        printf("identify failed\n");
        return;
    }

    uint16_t *buffer1 = malloc(1 * 256 * sizeof(uint16_t));
    read_ata(buffer1, ATA_PRIMARY, 1, 0);

    uint16_t *buffer2 = malloc(1 * 256 * sizeof(uint16_t));
    read_ata(buffer2, ATA_PRIMARY, 1, 0);

    for (int i = 0; i < 256; i++)
    {
        if (buffer1[i] != buffer2[i])
        {
            printf("test failed\n");
        }
    }
}
