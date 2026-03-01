#ifndef SPINLOCK_H
#define SPINLOCK_H
#include <stdatomic.h>

typedef struct spinlock
{
    atomic_flag mutex;
} spinlock;

void lock_spinlock(struct spinlock *lock);
void release_spinlock(struct spinlock *lock);

static inline void spinlock_lock(struct spinlock *lock)
{
    lock_spinlock(lock);
}

static inline void spinlock_unlock(struct spinlock *lock)
{
    release_spinlock(lock);
}

struct spinlock *new_spinlock();
#define SPINLOCK_INIT()                                                        \
                                                                               \
    (struct spinlock)                                                          \
    {                                                                          \
        .mutex = ATOMIC_FLAG_INIT                                              \
    }

#endif
