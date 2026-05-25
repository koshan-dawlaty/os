#include "device.h"
#include "lib/assert.h"
#include "lib/log.h"
#include "lib/refcount.h"
#include "wait_queue.h"
#include <bdev.h>
#include <cdev.h>
#include <err_ptr.h>
#include <fcntl.h>
#include <stdlib.h>
#include <vfs.h>

poll_flags dev_poll(struct file *f, struct wait_queue **queue)
{
    struct device *dev = f->entry->inode->fs_private;

    if (dev->type == DEV_BLOCK)
    {
        /* TODO: this is probably wrong */
        return POLL_READ | POLL_WRITE;
    }
    else if (dev->type == DEV_CHARACTER)
    {
        return cdev_poll(dev->character, queue);
    }
    else
    {
        UNREACHABLE("invalid device type %d", dev->type);
    }
}

void dev_inode_write(struct inode *inode, const u8 *buf, size_t n, size_t off)
{
    struct device *dev = inode->fs_private;
    /* TODO: see if we should use a file offset for block devices */

    if (dev->type == DEV_BLOCK)
    {
        bdev_write(dev->block, (u8 *)buf, n, off);
    }
    else if (dev->type == DEV_CHARACTER)
    {
        cdev_write(dev->character, (u8 *)buf, n);
    }
    else
    {
        UNREACHABLE("invalid device type %d", dev->type);
    }
}

void dev_inode_read(struct inode *inode, u8 *buf, size_t n, size_t off)
{
    struct device *dev = inode->fs_private;
    /* TODO: see if we should use a file offset for block devices */

    if (dev->type == DEV_BLOCK)
    {
        bdev_read(dev->block, (u8 *)buf, n, off);
    }
    else if (dev->type == DEV_CHARACTER)
    {
        cdev_read(dev->character, (u8 *)buf, n);
    }
    else
    {
        UNREACHABLE("invalid device type %d", dev->type);
    }
}

void dev_inode_free(struct inode *node)
{
    /* TODO: free device */
}

struct inode_functions device_functions = {.read_inode = dev_inode_read};
struct fs_type device_fs = {
    .is_permanent = true,
    .inode_functions =
        {
            .read_inode = dev_inode_read,
            .write_inode = dev_inode_write,
            .inode_free = dev_inode_free,
        },
    .name = "devfs",
    .file_operations = {file_read_default, file_write_default,
                        .poll = dev_poll},
};

struct inode *inode_dev_alloc(struct device *dev)
{

    /* TODO: dont alloc a superblock */
    struct superblock *sb = new_superblock(0, NULL, NULL, &device_fs);

    struct inode *out = new_inode(0, 0, dev, sb);

    return out;
}

int dev_read(struct file *file, char *buf, size_t n, size_t off)
{
    struct device *dev = file->entry->inode->fs_private;
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
    struct device *dev = file->entry->inode->fs_private;
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

const struct file_operations device_operations = {
    .read = dev_read, .write = dev_write, .poll = dev_poll};

/* TODO: this probably shouldnt just create devices in /dev/ instead maybe have
 * separate directories for device types */
/*
 * creates a device in the file system
 */
struct file *create_device(char *name, struct device *dev)
{
    struct pathname_lookup_flags flags = {
        true, DENTRY_FILE, {.node = inode_dev_alloc(dev)}};
    /* TODO: maybe not PATHNAME_LOOKUP_CREATE? */

    struct dentry *entry = lookup_path(name, flags);

    if (IS_ERR(entry))
    {
        return (void *)entry;
    }

    struct file *out = new_file(entry, O_RDWR);
    /* out->operations = device_operations; */

    return out;
}

/* TODO: bad name, does not actually add the device to the fdlist */
struct file *open_device(const char *devname)
{
    struct dentry *ent = lookup_path(devname, PATHNAME_LOOKUP_FIND);

    if (IS_ERR(ent))
    {
        return (void *)GET_ERR(ent);
    }

    file *out = new_file(ent, O_RDWR);
    out->operations = device_operations;
    return out;
}
