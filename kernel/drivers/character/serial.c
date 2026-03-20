#include "serial.h"
#include "device.h"
#include <cdev.h>
#include <stdio.h>
#include <types.h>

struct serial_cdev
{
    u16 port;
};

int serial_cdev_read(struct cdev *cdev, u8 *buf, size_t num)
{
    struct serial_cdev *dev = cdev->cdev_private;
    for (size_t i = 0; i < num; i++)
    {
        buf[i] = serial_read(dev->port);
    }
    return num;
}

int serial_cdev_write(struct cdev *cdev, u8 *buf, size_t num)
{
    struct serial_cdev *dev = cdev->cdev_private;
    for (size_t i = 0; i < num; i++)
    {
        serial_write(dev->port, buf[i]);
    }
    return num;
}

struct cdev_functions serial_operations = {.read = serial_cdev_read,
                                           .write = serial_cdev_write};

struct device *serial_cdev_create(u16 port)
{
    /* TODO: if only the port is needed it could be stored in the
     * cdev->cdev_private field, which would save a memory access */
    struct serial_cdev *dev = malloc(sizeof(struct serial_cdev));
    dev->port = port;
    struct device *out = new_cdev(dev, serial_operations);
    return out;
}
