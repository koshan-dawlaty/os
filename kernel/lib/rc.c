#include <lib/refcount.h>

void refcount_get(struct refcount *rc)
{
    rc->count++;
}

void refcount_drop(struct refcount *rc, void (*drop)(struct refcount *))
{

    rc->count--;

    if (rc->count <= 0)
    {
        drop(rc);
        /* the user (probably) frees rc when freeing the struct containing rc,
         * so we dont have to free it */
    }
}
