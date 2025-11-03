#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include <kernel/types.h>
#include <lib/refcount.h>
#include <stddef.h>

struct bdev;

struct bdev_functions
{
    int (*read_blocks)(struct bdev *dev, u8 *buf, size_t num_blocks,
                       u64 block_num);
    int (*write_blocks)(struct bdev *dev, u8 *buf, size_t num_blocks,
                        u64 block_num);
    void (*free)(struct bdev *dev); /* called automatically when the device is
                                       no longer in use */
};

struct bdev
{
    struct bdev_functions functions;
    void *bdev_private;
    size_t block_size;
    refcount *rc;
};

/*
 * read from a block device
 * @offset bytes into the device to read from (not a sector offset)
 */
int bdev_read(struct bdev *dev, u8 *buf, size_t num, u64 offset);

/*
 * write to a block device
 * @offset bytes into the device to write to (not a sector offset)
 */
int bdev_write(struct bdev *dev, u8 *buf, size_t num, u64 offset);

struct bdev *new_bdev(struct bdev_functions functions, size_t block_size,
                      void *private);

#endif
