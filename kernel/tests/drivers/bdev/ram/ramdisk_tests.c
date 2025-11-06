#include "bdev.h"
#include <compiler.h>
#include <drivers/block/ram/ramdisk.h>
#include <stdlib.h>
#include <string.h>
#include <tests/tests.h>

struct bdev *test_ramdisk = NULL;

void __constructor init_ramdisk_tests()
{
    test_ramdisk = new_ramdisk();
}

TEST(ramdisk_read_small, {
    size_t size = 400;
    size_t offset = 100;
    u8 *buf = malloc(size);

    for (size_t i = 0; i < size; i++)
    {
        buf[i] = i % 255;
    }

    bdev_write(test_ramdisk, buf, size, offset);

    u8 *read_buf = malloc(size);
    bdev_read(test_ramdisk, read_buf, size, offset);

    for (size_t i = 0; i < size; i++)
    {
        ASSERT_EQ(buf[i], read_buf[i],
                  "data written %d not equal to data read %d at index %d",
                  buf[i], read_buf[i], i);
    }

    free(buf);
    free(read_buf);
})

TEST(ramdisk_read_medium, {
    size_t size = 300;
    size_t offset = 8093;
    u8 *buf = malloc(size);

    for (size_t i = 0; i < size; i++)
    {
        buf[i] = i % 255;
    }

    bdev_write(test_ramdisk, buf, size, offset);

    u8 *read_buf = malloc(size);
    bdev_read(test_ramdisk, read_buf, size, offset);

    for (size_t i = 0; i < size; i++)
    {
        ASSERT_EQ(buf[i], read_buf[i],
                  "data written %d not equal to data read %d at index %d",
                  buf[i], read_buf[i], i);
    }

    free(buf);
    free(read_buf);
})
