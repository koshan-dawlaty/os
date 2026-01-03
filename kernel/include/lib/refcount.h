#ifndef RC_H
#define RC_H

struct refcount
{
    volatile int count;
};

#define REFCOUNT_INIT() (struct refcount){.count = 1}

void refcount_get(struct refcount *rc);
void refcount_drop(struct refcount *rc, void (*drop)(struct refcount *));

#endif
