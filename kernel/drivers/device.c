#include "device.h"
#include <bdev.h>
#include <cdev.h>
#include <sys/types.h>

struct device *new_character_device(struct cdev *dev)
{
    struct device *out = malloc(sizeof(struct device));

    out->type = DEV_CHARACTER;
    /* TODO */
    out->num = 1234;

    out->character = dev;

    return out;
}

struct device *new_block_device(struct bdev *dev)
{
    struct device *out = malloc(sizeof(struct device));

    out->type = DEV_BLOCK;
    /* TODO */
    out->num = 1234;

    out->block = dev;

    return out;
}
