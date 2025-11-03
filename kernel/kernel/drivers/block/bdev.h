#ifndef BLOCK_DEVICE_H
#define BLOCK_DEVICE_H

#include <kernel/types.h>
#include <lib/refcount.h>
#include <stddef.h>

struct bdev;

typedef struct
{
    int (*read)(struct bdev *dev, u8 *buf, size_t num, u64 offset);
    int (*write)(struct bdev *dev, u8 *buf, size_t num, u64 offset);
    void (*free)(struct bdev *dev); /* called automatically when the device is
                                       no longer in use */
} bdev_functions;

struct bdev
{
    bdev_functions functions;
    void *bdev_private;
    refcount *rc;
};

/*
 * read from a block device
 * @offset bytes into the device to read from (not a sector offset)
 */
static inline int bdev_read(struct bdev *dev, u8 *buf, size_t num, u64 offset)
{
    return dev->functions.read(dev, buf, num, offset);
}

/*
 * write to a block device
 * @offset bytes into the device to write to (not a sector offset)
 */
static inline int bdev_write(struct bdev *dev, u8 *buf, size_t num, u64 offset)
{
    return dev->functions.write(dev, buf, num, offset);
}

struct bdev *new_bdev(bdev_functions functions, void *private);

#endif
