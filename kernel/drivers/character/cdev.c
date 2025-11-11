#include <cdev.h>

int cdev_read(struct cdev *dev, u8 *buf, size_t len)
{
    return dev->functions.read(dev, buf, len);
}

int cdev_write(struct cdev *dev, u8 *buf, size_t len)
{
    return dev->functions.write(dev, buf, len);
}

struct device *new_cdev(void *private, struct cdev_functions functions)
{
    struct cdev *dev = malloc(sizeof(struct cdev));

    dev->cdev_private = private;
    dev->functions = functions;

    return new_character_device(dev);
}
