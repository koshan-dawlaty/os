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

/* TODO: this probably shouldnt just create devices in /dev/ */
/*
 * creates a device in the file system
 */
file *create_device(char *name, struct device *dev)
{
    return NULL;
}
