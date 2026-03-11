#include "assert.h"
#include <bdev.h>
#include <cdev.h>
#include <err_ptr.h>
#include <fcntl.h>
#include <stdlib.h>
#include <vfs.h>

dentry *new_dentry_dev(char *name, dentry *parent, struct device *dev)
{
    dentry *out = malloc(sizeof(dentry));
    out->dev = dev;
    out->name = name;
    out->parent = parent;
    out->type = DENTRY_DEVICE;
    return out;
}

int dev_read(struct file *file, char *buf, size_t n, size_t off)
{
    struct device *dev = file->entry->dev;
    /* TODO: see if we should use a file offset for block devices */

    if (dev->type == DEV_BLOCK)
    {
        return bdev_read(dev->block, (u8 *)buf, n, off);
    }
    else if (dev->type == DEV_CHARACTER)
    {
        return cdev_read(dev->character, (u8 *)buf, n);
    }
    else
    {
        UNREACHABLE("invalid device type %d", dev->type);
    }
}

int dev_write(struct file *file, const char *buf, size_t n, size_t off)
{
    struct device *dev = file->entry->dev;
    /* TODO: see if we should use a file offset for block devices */

    if (dev->type == DEV_BLOCK)
    {
        return bdev_write(dev->block, (u8 *)buf, n, off);
    }
    else if (dev->type == DEV_CHARACTER)
    {
        return cdev_write(dev->character, (u8 *)buf, n);
    }
    else
    {
        UNREACHABLE("invalid device type %d", dev->type);
    }
}

const struct file_operations device_operations = {.read = dev_read,
                                                  .write = dev_write};

/* TODO: this probably shouldnt just create devices in /dev/ instead maybe have
 * separate directories for device types */
/*
 * creates a device in the file system
 */
struct file *create_device(char *name, struct device *dev)
{
    /* TODO: maybe not PATHNAME_LOOKUP_CREATE? */
    struct dentry *entry = lookup_path(name, PATHNAME_LOOKUP_CREATE_DEV(dev));

    if (IS_ERR(entry))
    {
        return (void *)entry;
    }

    entry->type = DENTRY_DEVICE;
    entry->dev = dev;

    struct file *out = new_file(entry, O_RDWR);
    out->operations = device_operations;

    return out;
}

/* TODO: bad name, does not actually add the device to the fdlist */
file *open_device(const char *devname)
{
    dentry *ent = lookup_path(devname, PATHNAME_LOOKUP_FIND);

    if (IS_ERR(ent))
    {
        return (void *)GET_ERR(ent);
    }

    file *out = new_file(ent, O_RDWR);
    out->operations = device_operations;
    return out;
}
