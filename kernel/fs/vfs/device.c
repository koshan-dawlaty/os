#include <stdlib.h>
#include <vfs.h>

dentry *new_dentry_dev(char *name, dentry *parent, struct bdev *dev)
{
    dentry *out = malloc(sizeof(dentry));
    out->dev = dev;
    out->name = name;
    out->parent = parent;
    out->type = DENTRY_DEVICE;
    return out;
}
