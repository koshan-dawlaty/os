#include "ata.h"
#include "interrupts/pic.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void init_ata()
{
    IRQ_clear_mask(14);
    IRQ_clear_mask(15);

    ata_drive *drive = get_ata_drive(0);

    if (drive != NULL)
    {

        uint8_t *buffer = malloc(512);

        read_drive(drive, buffer, 512, 0);
        printf("%d\n", drive->is_dma);
        /* read_ata_pio(buffer, ATA_PRIMARY, 1, 0); */

        for (int i = 0; i < 512; i++)
        {
            printf("%p", buffer[i]);
        }
    }
}
