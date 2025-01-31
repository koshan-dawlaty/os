#include "ata/ata.h"
#include "tests/tests.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

TEST(pio_read, {
    uint16_t *buffer = malloc(512);

    read_ata_pio(buffer, ATA_PRIMARY, 1, 0);

    ASSERT_EQ(buffer[0], 0x3231);
    ASSERT_EQ(buffer[1], 0x3433);
});

TEST(pio_write, {
    size_t sectors = 2;
    size_t lba = 4;

    uint16_t *buffer = malloc(sectors * 512);

    write_ata_pio(ATA_PRIMARY, lba, buffer, sectors * 512);

    uint16_t *result_buffer = malloc(sectors * 512);
    read_ata_pio(result_buffer, ATA_PRIMARY, sectors, lba);

    for (size_t i = 0; i < 512; i++)
    {
        ASSERT_EQ(buffer[i], result_buffer[i]);
    }
})
