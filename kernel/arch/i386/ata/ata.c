#include "ata.h"
#include "interrupts/idt.h"
#include "interrupts/pic.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>

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

    ata_drive_select(io_offset, control_offset, 0xA0);
    outb(io_offset + ATA_SECTOR_COUNT, 0);
    outb(io_offset + ATA_LBA_LO, 0);
    outb(io_offset + ATA_LBA_MID, 0);
    outb(io_offset + ATA_LBA_HI, 0);
    // IDENTIFY command
    outb(io_offset + ATA_COMMAND, 0xEC);

    // should be alternate status
    uint8_t val = inb(control_offset + ATA_ALT_STATUS);
    // TODO: DELAY HERE
    /* printf("%p\n", val); */
    while (val & (1 << 7))
    {
        if (inb(io_offset + ATA_LBA_MID) != 0 ||
            inb(io_offset + ATA_LBA_HI) != 0)
        {
            return 1;
        }

        val = inb(control_offset + ATA_ALT_STATUS);
    }

    return 0;
}

ISR(ata_isr, {
    printf("ata isr\n");
    PIC_sendEOI(14);
})

void init_ata()
{
    if (!send_identify(ATA_PRIMARY))
    {
        printf("identify succeeded!\n");
    }
    else
    {
        printf("identify failed\n");
        return;
    }
}
