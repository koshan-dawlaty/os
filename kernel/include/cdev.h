#ifndef CDEV_H
#define CDEV_H

#include "device.h"
#include <kernel/types.h>
#include <stddef.h>
#include <stdlib.h>

struct cdev;
struct cdev_functions
{
    int (*read)(struct cdev *dev, u8 *buf, size_t len);
    int (*write)(struct cdev *dev, u8 *buf, size_t len);
};

struct cdev
{
    struct cdev_functions functions;
    void *cdev_private;
};

int cdev_read(struct cdev *dev, u8 *buf, size_t len);
int cdev_write(struct cdev *dev, u8 *buf, size_t len);

struct device *new_cdev(void *private, struct cdev_functions functions);

#endif
