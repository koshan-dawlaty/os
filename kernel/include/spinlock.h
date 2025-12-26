#ifndef SPINLOCK_H
#define SPINLOCK_H
#include <stdatomic.h>

typedef struct spinlock
{
    atomic_flag mutex;
} spinlock;

void lock_spinlock(spinlock *lock);
void release_spinlock(spinlock *lock);
spinlock *new_spinlock();
#define SPINLOCK_INIT()                                                        \
                                                                               \
    (struct spinlock)                                                          \
    {                                                                          \
        .mutex = ATOMIC_FLAG_INIT                                              \
    }

#endif
