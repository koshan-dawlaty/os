#include "spinlock.h"
#include <stdatomic.h>
#include <stdlib.h>

spinlock *new_spinlock()
{

    spinlock *out = malloc(sizeof(spinlock));
    atomic_flag_clear(&out->mutex);

    return out;
}

void lock_spinlock(spinlock *lock)
{
    while (
        atomic_flag_test_and_set_explicit(&lock->mutex, memory_order_acquire))
    {
        __builtin_ia32_pause();
    }
}

void release_spinlock(spinlock *lock)
{
    atomic_flag_clear(&lock->mutex);
}
