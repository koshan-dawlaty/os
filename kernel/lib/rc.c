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

    if (rc->count <= 0)
    {
        drop(rc);
        /* the user (probably) frees rc when freeing the struct containing rc,
         * so we dont have to free it */
    }
}
