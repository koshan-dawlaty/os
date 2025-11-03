#include "ata/ata.h"
#include "drivers/block/bdev.h"
#include "tests/tests.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct bdev *pio_test_drive = NULL;

void init_pio_tests()
{
    if (pio_test_drive == NULL)
    {
        pio_test_drive = get_ata_drive(0);
    }
}

/*
TEST(pio_write, {
    init_pio_tests();

    size_t sectors = 2;
    size_t lba = 4;

    uint8_t *inital_content = malloc(sectors * 512);

    read_drive(drive, inital_content, sectors * 512, lba);

    uint8_t *buffer = malloc(sectors * 512);

    for (size_t i = 0; i < sectors * 512; i++)
    {
        buffer[i] = (i % 10) + 0x30;
    }

    write_drive(drive, buffer, sectors * 512, lba);

    uint8_t *result_buffer = malloc(sectors * 512);
    read_drive(drive, result_buffer, sectors * 512, lba);

    for (size_t i = 0; i < sectors * 512; i++)
    {
        ASSERT_EQ(buffer[i], result_buffer[i]);
    }

    write_drive(drive, inital_content, sectors * 512, lba);

    free(buffer);
    free(result_buffer);
    free(inital_content);
})
*/

TEST(pio_high_lba, {
    init_pio_tests();

    size_t sectors = 2;
    size_t lba = 1000;

    uint8_t *inital_content = malloc(sectors * 512);

    bdev_read(pio_test_drive, inital_content, sectors * 512, lba * 512);

    uint8_t *buffer = malloc(sectors * 512);

    for (size_t i = 0; i < sectors * 512; i++)
    {
        buffer[i] = (i % 10) + 0x30;
    }

    bdev_write(pio_test_drive, buffer, sectors * 512, lba * 512);

    uint8_t *result_buffer = malloc(sectors * 512);
    bdev_read(pio_test_drive, result_buffer, sectors * 512, lba * 512);

    for (size_t i = 0; i < sectors * 512; i++)
    {
        ASSERT_EQ(buffer[i], result_buffer[i]);
    }

    bdev_write(pio_test_drive, inital_content, sectors * 512, lba * 512);

    free(buffer);
    free(result_buffer);
    free(inital_content);
});
