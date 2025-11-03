#include "bdev.h"
#include <container_of.h>
#include <lib/refcount.h>
#include <stdlib.h>

struct bdev *new_bdev(bdev_functions functions, void *private)
{
    struct bdev *out = malloc(sizeof(struct bdev));

    out->bdev_private = private;
    out->functions = functions;
    out->rc = new_refcount();

    return out;
}

void get_bdev(struct bdev *dev)
{
    refcount_get(dev->rc);
}

void bdev_refcount_drop(refcount *rc)
{
    struct bdev *device = containerof(rc, struct bdev, rc);
    device->functions.free(device);
    free(device);
}

void drop_bdev(struct bdev *dev)
{
    refcount_drop(dev->rc, bdev_refcount_drop);
}
