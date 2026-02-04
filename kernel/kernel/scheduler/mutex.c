#include "lib/array.h"
#include <fcntl.h>
#include <kernel/syscall.h>
#include <lib/array.h>
#include <mutex.h>
#include <panic.h>
#include <sched.h>
#include <scheduler.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <utils.h>
#include <vfs.h>

extern void display_queue();

void mutex_lock(struct mutex *m)
{
    irq_state_t state = irq_save_disable();

    if (!m->locked)
    {
        /* printf("lock 1, %p\n", get_current_task()); */
        m->locked = true;
    }
    else
    {
        /* printf("lock 2, %p\", get_current_task()); */
        struct task *current = get_current_task();
        /* printf("before:\n"); */
        remove_task(current);
        /* display_queue(); */
        task_queue_push(&m->queue, current);
        /* display_queue(); */
        /* printf("after:\n"); */
        /* display_queue(); */

        sched_yield();
    }

    irq_restore(state);
}

void mutex_unlock(struct mutex *m)
{
    irq_state_t state = irq_save_disable();
    if (!m->locked)
    {
        panic("mutex unlock may have been called twice on the same mutex!");
    }

    if (m->queue.head == NULL)
    {
        /* printf("unlock 1, %p\n", get_current_task()); */
        m->locked = false;
    }
    else
    {
        /* printf("unlock 2, %p\n", get_current_task()); */
        struct task *temp = task_queue_pop(&m->queue);
        add_task(temp);
        schedule();
    }

    irq_restore(state);
}

struct array user_mutex_array = ARRAY_INIT();

int syscall_mutex_create()
{
    struct mutex mutex = MUTEX_INIT();
    struct mutex *element = malloc(sizeof(struct mutex));
    *element = mutex;
    size_t index =
        array_add(&user_mutex_array, element, sizeof(struct mutex *));
    return index;
}
SYSCALL_DEFINE0(syscall_mutex_create, SYS_mutex_create);

void syscall_mutex_lock(int id)
{
    struct mutex *mutex =
        array_get(&user_mutex_array, id, sizeof(struct mutex *));

    mutex_lock(mutex);
}
SYSCALL_DEFINE1(syscall_mutex_lock, SYS_mutex_lock);

void syscall_mutex_unlock(int id)
{
    struct mutex *mutex =
        array_get(&user_mutex_array, id, sizeof(struct mutex *));

    mutex_unlock(mutex);
}
SYSCALL_DEFINE1(syscall_mutex_unlock, SYS_mutex_unlock);
