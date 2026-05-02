#ifndef SPINLOCK_H
#define SPINLOCK_H
#include <kernel/preempt.h>
#include <stdatomic.h>

struct spinlock
{
#ifdef CONFIG_SMP
    atomic_flag mutex;
#endif
};

static inline void spinlock_lock(struct spinlock *lock)
{
#ifdef CONFIG_SMP
    while (
        atomic_flag_test_and_set_explicit(&lock->mutex, memory_order_acquire))
    {
        __builtin_ia32_pause();
    }
#else
    preempt_disable();
#endif
}

static inline void spinlock_unlock(struct spinlock *lock)
{
#ifdef CONFIG_SMP
    atomic_flag_clear(&lock->mutex);
#else
    preempt_enable();
#endif
}

#ifdef CONFIG_SMP
#define SPINLOCK_INIT()                                                        \
    (struct spinlock)                                                          \
    {                                                                          \
        .mutex = ATOMIC_FLAG_INIT                                              \
    }
#else
#define SPINLOCK_INIT()                                                        \
    (struct spinlock)                                                          \
    {                                                                          \
    }
#endif

static inline void spinlock_init(struct spinlock *lock)
{
    *lock = SPINLOCK_INIT();
}

#endif
