#include "spinlock.h"
#include <stdatomic.h>
#include <stdlib.h>

spinlock *new_spinlock()
{
    spinlock *out = malloc(sizeof(spinlock));
    out->mutex = (atomic_flag)ATOMIC_FLAG_INIT;

    return out;
}

void lock_spinlock(spinlock *lock)
{
    while (atomic_flag_test_and_set(&lock->mutex))
    {
        __builtin_ia32_pause();
    }
}

void release_spinlock(spinlock *lock)
{
    atomic_flag_clear(&lock->mutex);
}
