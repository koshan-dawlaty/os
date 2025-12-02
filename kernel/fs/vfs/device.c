#include "err.h"
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

/* TODO: this probably shouldnt just create devices in /dev/ instead maybe have
 * separate directories for device types */
/*
 * creates a device in the file system
 */
file *create_device(char *name, struct device *dev)
{
    /* TODO: maybe not PATHNAME_LOOKUP_CREATE? */
    dentry *entry = lookup_path(name, PATHNAME_LOOKUP_CREATE_DEV(dev));

    if (IS_ERR(entry))
    {
        return ERR_PTR(entry);
    }

    entry->type = DENTRY_DEVICE;
    entry->dev = dev;

    return new_file(entry);
}
