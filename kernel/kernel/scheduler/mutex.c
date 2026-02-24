#include "lib/array.h"
#include "unistd.h"
#include <fcntl.h>
#include <kernel/syscall.h>
#include <lib/array.h>
#include <mutex.h>
#include <panic.h>
#include <sched.h>
#include <scheduler.h>
#include <stdlib.h>
#include <string.h>
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
        printf("removing task\n");
        /* printf("lock 2, %p\", get_current_task()); */
        struct task *current = get_current_task();
        /* printf("before:\n"); */

        save_registers(&current->regs);
        task_queue_push(&m->queue, current);

        remove_task(current);

        /* display_queue(); */
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
        printf("adding task\n");
        /* printf("unlock 2, %p\n", get_current_task()); */
        struct task *temp = task_queue_pop(&m->queue);
        add_task(temp);
        schedule();
    }

    irq_restore(state);
}

/*
 * all of this code is pretty bad, maybe dont use the array, this might result
 * in weird things with multiple tasks allocating mutexes
 */
struct array user_mutex_array = ARRAY_INIT();

SYSCALL_DEFINE0(SYS_mutex_create, int)
{
    struct mutex mutex = MUTEX_INIT();
    struct mutex *element = malloc(sizeof(struct mutex));
    memcpy(element, &mutex, sizeof(struct mutex));
    size_t index =
        array_add(&user_mutex_array, &element, sizeof(struct mutex *));
    return index;
}

SYSCALL_DEFINE1(SYS_mutex_lock, int, int, id)
{
    struct mutex *mutex = *(struct mutex **)array_get(&user_mutex_array, id,
                                                      sizeof(struct mutex *));
    mutex_lock(mutex);

    return 0;
}

SYSCALL_DEFINE1(SYS_mutex_unlock, int, int, id)
{
    struct mutex *mutex = *(struct mutex **)array_get(&user_mutex_array, id,
                                                      sizeof(struct mutex *));
    mutex_unlock(mutex);

    return 0;
}
