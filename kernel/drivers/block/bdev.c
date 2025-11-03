#include <container_of.h>
#include <drivers/block/bdev.h>
#include <lib/refcount.h>
#include <panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct bdev *new_bdev(struct bdev_functions functions, size_t block_size,
                      void *private)
{
    struct bdev *out = malloc(sizeof(struct bdev));

    out->bdev_private = private;
    out->functions = functions;
    out->block_size = block_size;
    out->rc = new_refcount();

    return out;
}

void get_bdev(struct bdev *dev)
{
    refcount_get(dev->rc);
}

void bdev_refcount_drop(refcount *rc)
{
    struct bdev *device = containerof(rc, struct bdev, rc);
    device->functions.free(device);
    free(device);
}

void drop_bdev(struct bdev *dev)
{
    refcount_drop(dev->rc, bdev_refcount_drop);
}

int bdev_write(struct bdev *dev, u8 *buf, size_t num, u64 offset)
{
    size_t block_size = dev->block_size;
    size_t bytes_written = 0;

    if (offset % block_size != 0)
    {
        u8 *block = malloc(block_size);
        dev->functions.read_blocks(dev, block, 1, offset / block_size);

        size_t bytes_remaining = block_size - (offset % block_size);

        if (num < bytes_remaining)
        {
            bytes_remaining = num;
        }

        memcpy(block + (offset % block_size), buf, bytes_remaining);
        dev->functions.write_blocks(dev, block, 1, offset / block_size);

        bytes_written += bytes_remaining;

        free(block);
    }

    offset += bytes_written;
    num -= bytes_written;

    size_t num_blocks = (num) / block_size;
    size_t block_num = (offset) / block_size;

    printf("num_blocks:%d\n", num_blocks);
    printf("block_num:%d\n", block_num);

    dev->functions.write_blocks(dev, buf + bytes_written, num_blocks,
                                block_num);
    bytes_written += num_blocks * block_size;

    if (num % block_size != 0)
    {
        u8 *block = malloc(block_size);
        dev->functions.read_blocks(dev, block, 1, (offset + num) / block_size);

        size_t bytes_remaining = block_size - (num % block_size);

        memcpy(block, buf + bytes_written, bytes_remaining);

        dev->functions.write_blocks(dev, block, 1, (offset + num) / block_size);

        free(block);
    }

    return 1234;
}

/* TODO: fix return values */
int bdev_read(struct bdev *dev, u8 *buf, size_t num, u64 offset)
{
    size_t block_size = dev->block_size;
    size_t bytes_written = 0;

    /* if not at the start of a block read part of the block into buf */
    if (offset % block_size != 0)
    {
        u8 *block = malloc(block_size);
        dev->functions.read_blocks(dev, block, 1, offset / block_size);

        size_t bytes_remaining = block_size - (offset % block_size);

        if (num < bytes_remaining)
        {
            bytes_remaining = num;
        }

        /* copy the partial block into the buffer */
        memcpy(buf, block + (offset % block_size), bytes_remaining);

        bytes_written += bytes_remaining;

        free(block);
    }

    offset += bytes_written;
    num -= bytes_written;

    size_t num_blocks = (num) / block_size;
    size_t block_num = (offset) / block_size;

    dev->functions.read_blocks(dev, buf + bytes_written, num_blocks, block_num);
    bytes_written += num_blocks * block_size;

    if (num % block_size != 0)
    {
        u8 *block = malloc(block_size);
        dev->functions.read_blocks(dev, block, 1, (offset + num) / block_size);

        size_t bytes_remaining = block_size - (num % block_size);

        memcpy(buf + bytes_written, block, bytes_remaining);

        free(block);
    }

    return 1234;
}
