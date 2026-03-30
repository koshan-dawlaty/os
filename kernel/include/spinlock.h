#ifndef SPINLOCK_H
#define SPINLOCK_H
#include <stdatomic.h>

struct spinlock
{
    atomic_flag mutex;
};

static inline void spinlock_lock(struct spinlock *lock)
{
    while (
        atomic_flag_test_and_set_explicit(&lock->mutex, memory_order_acquire))
    {
        __builtin_ia32_pause();
    }
}

static inline void spinlock_unlock(struct spinlock *lock)
{
    atomic_flag_clear(&lock->mutex);
}

#define SPINLOCK_INIT()                                                        \
                                                                               \
    (struct spinlock)                                                          \
    {                                                                          \
        .mutex = ATOMIC_FLAG_INIT                                              \
    }

static inline void spinlock_init(struct spinlock *lock)
{
    *lock = SPINLOCK_INIT();
}

#endif
