#ifndef DEVICE_H
#define DEVICE_H

#include <sys/types.h>

enum dev_type
{
    DEV_BLOCK = 1,
    DEV_CHARACTER = 2,
};

struct device
{
    dev_t num;
    enum dev_type type;

    union
    {
        struct bdev *block;
        struct cdev *character;
    };
};

struct device *new_character_device(struct cdev *dev);
struct device *new_block_device(struct bdev *dev);

#endif
