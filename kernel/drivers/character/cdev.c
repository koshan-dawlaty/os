#include <cdev.h>
#include <vfs.h>
#include <wait_queue.h>

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

poll_flags cdev_poll(struct cdev *dev, struct wait_queue **queue)
{
    return dev->functions.poll(dev, queue);
}
