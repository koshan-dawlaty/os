#include "ata.h"
#include "interrupts/idt.h"
#include "interrupts/pic.h"
#include "stdlib.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>

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

    if (result & (1 << 7))
    {
        // drive still has status busy
    }
}

// TODO: may hang if drive is not present

/*
 * returns 0 if succeeded
 *
 * returns 1 if drive is not ata
 */
int send_identify(uint16_t io_offset, uint16_t control_offset)
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
            return 1;
        }
    } while (status & (1 << 7) && !(status & (1 << 3)));

    // check DRQ
    do
    {
        status = inb(io_offset + ATA_STATUS);
    } while (!(status & (1 << 3)));

    return 0;
}

ISR(ata_isr, {
    printf("ata isr\n");
    PIC_sendEOI(14);
})

void read_ata(uint16_t io_offset, uint16_t control_offset, uint32_t sectors,
              uint32_t lba)
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

        printf("%p\n", status);
    }

    printf("lba %p\n", (unsigned char)lba);

    /* outb(io_offset + ATA_DRIVE_HEAD, */
    /*      0xE0 | ((lba >> 24) & 0x0F)); // Select drive, set high bits of LBA
     */

    ata_drive_select(io_offset, control_offset, 0xE0 | ((lba >> 24) & 0x0F));

    outb(io_offset + ATA_ERROR, 0);
    outb(io_offset + ATA_SECTOR_COUNT, (unsigned char)sectors);
    outb(io_offset + ATA_LBA_LO, (unsigned char)lba);
    outb(io_offset + ATA_LBA_MID, (unsigned char)lba >> 8);
    outb(io_offset + ATA_LBA_HI, (unsigned char)lba >> 16);

    // READ SECTORS
    outb(io_offset + ATA_COMMAND, 0x20);

    printf("%p\n", inw(io_offset + ATA_DATA));
    print_all_regs(ATA_PRIMARY_IO);

    for (size_t i = 0; i < 256 * sectors; i++)
    {

        uint8_t status;
        do
        {

            status = inb(io_offset + ATA_STATUS);
            printf("%p\n", status);

        } while ((status & (1 << 7) && !(status & (1 << 3))));

        uint16_t data = inw(io_offset + ATA_DATA);

        printf("%p\n", data);
    }

    printf("test\n");
}

void init_ata()
{

    IRQ_clear_mask(14);
    IRQ_clear_mask(15);

    if (!send_identify(ATA_PRIMARY))
    {
        printf("identify succeeded!\n");
    }
    else
    {
        printf("identify failed\n");
        return;
    }

    read_ata(ATA_PRIMARY, 1, 0);
}
