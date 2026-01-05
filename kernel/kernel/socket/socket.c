#include "sched.h"
#include "utils.h"
#include "vfs.h"
#include <lib/list.h>
#include <socket.h>
#include <stdlib.h>
#include <string.h>
#include <task_queue.h>

struct unix_socket *unix_socket_alloc()
{
    struct unix_socket *out = malloc(sizeof(struct unix_socket));

    out->buffer = ARRAY_INIT();
    out->send_queue = TASK_QUEUE_INIT();
    out->recv_queue = TASK_QUEUE_INIT();

    return out;
}

int unix_socket_recv(struct unix_socket *sock, void *buf, size_t len)
{

    if (sock->send_queue.head == NULL)
    {
        IRQ_OFF({
            struct task *current = get_current_task();
            task_queue_push(&sock->recv_queue, current);
            save_registers(&current->regs);
            remove_task(current);
        })
    }
    else
    {
        add_task(task_queue_pop(&sock->send_queue));
    }

    /* TODO: this should remove data and not go out of bounds */
    void *sent = array_pop(&sock->buffer, len);
    memcpy(buf, sent, len);

    return 0;
}

int unix_socket_send(struct unix_socket *sock, void *buf, size_t len)
{
    array_add(&sock->buffer, buf, len);
    if (sock->recv_queue.head == NULL)
    {

        IRQ_OFF({
            struct task *current = get_current_task();
            task_queue_push(&sock->send_queue, current);
            save_registers(&current->regs);
            remove_task(current);
        })
    }
    else
    {
        add_task(task_queue_pop(&sock->recv_queue));
        schedule();
    }

    return 0;
}

int unix_socket_listen(struct unix_socket *sock, size_t backlog)
{

    if (sock->state != SOCKET_DEFAULT)
    {
        return -1; /* TODO: real error code */
    }
    sock->state = SOCKET_LISTEN;
    sock->listener.backlog = backlog;

    return 0;
}

int unix_socket_accept(struct unix_socket *sock, struct unix_socket *client)
{
    if (sock->state != SOCKET_LISTEN)
    {
        return -1;
    }

    if (sock->listener.waiting == NULL)
    {
        return -1; /* TODO: maybe block? */
    }

    *client = *sock->listener.waiting;
    struct list2 *list = &client->listener.waiting->connected.list;

    if (list->next == NULL)
    {
        sock->listener.waiting = NULL;
        return 0;
    }

    struct list2 *next = list->next;
    sock->listener.waiting = LIST_ENTRY(
        next, struct unix_socket,
        connected.list); /* this might be incorrect way to use containerof() */

    return 0;
}

int unix_socket_connect(struct unix_socket *client, struct unix_socket *server)
{
    if (client->state != SOCKET_DEFAULT)
    {
        return -1; /* TODO: real error code */
    }
    client->state = SOCKET_CONNECTED;
    client->connected.list = LIST2_INIT();
    struct unix_socket *waiting_queue_head = server->listener.waiting;
    if (waiting_queue_head == NULL)
    {
        server->listener.waiting = client;
        return 0;
    }

    client->connected.list.next = &waiting_queue_head->connected.list;
    waiting_queue_head->connected.list.prev = &client->connected.list;

    return 0;
}
