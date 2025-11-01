#include <lib/refcount.h>
#include <stdlib.h>

refcount *new_refcount()
{
    refcount *out = malloc(sizeof(refcount));
    out->count = 1;

    return out;
}

void refcount_get(refcount *rc)
{
    rc->count++;
}

void refcount_drop(refcount *rc, void (*drop)(refcount *))
{

    rc->count--;

    if (rc->count < 0)
    {
        drop(rc);
        /* TODO maybe the user wants to free this */
        free(rc);
    }
}
