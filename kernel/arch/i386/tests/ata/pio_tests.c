#include "ata/ata.h"
#include "tests/tests.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

ata_drive *drive = NULL;

void init_pio_tests()
{
    if (drive == NULL)
    {
        drive = get_ata_drive(0);
    }
}

TEST(pio_read, {
    init_pio_tests();

    uint8_t *buffer = malloc(512);

    read_drive(drive, buffer, 512, 0);
    /* read_ata_pio(buffer, ATA_PRIMARY, 1, 0); */

    for (int i = 0; i < 9; i++)
    {
        /* ASSERT_EQ(buffer[i], 0x31 + i); */
    }
});

TEST(pio_write, {
    init_pio_tests();

    size_t sectors = 2;
    size_t lba = 4;

    uint8_t *buffer = malloc(sectors * 512);

    for (size_t i = 0; i < sectors * 512; i++)
    {
        buffer[i] = 'a';
    }

    write_drive(drive, buffer, sectors * 512, 0);

    uint8_t *result_buffer = malloc(sectors * 512);
    read_drive(drive, result_buffer, sectors * 512, 0);

    for (size_t i = 0; i < sectors * 512; i++)
    {
        /* ASSERT_EQ(buffer[i], result_buffer[i]); */
    }
})
