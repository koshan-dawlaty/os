#ifndef RC_H
#define RC_H

typedef struct
{
    volatile int count;
} refcount;

#define REFCOUNT_INIT() (refcount){.count = 1}

refcount *new_refcount();

void refcount_get(refcount *rc);
void refcount_drop(refcount *rc, void (*drop)(refcount *));

#endif
