#include "kernel/log.h"
#include "panic.h"
#include "spinlock.h"
#include "unistd.h"
#include <errno.h>
#include <kernel/syscall.h>
#include <lib/array.h>
#include <lib/list.h>
#include <sched.h>
#include <socket.h>
#include <spinlock.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <task_queue.h>
#include <utils.h>
#include <vfs.h>
#include <wait_queue.h>

struct array unix_socket_array = ARRAY_INIT();
struct spinlock unix_socket_array_lock = SPINLOCK_INIT();

struct unix_socket *unix_socket_lookup(int id)
{
    spinlock_lock(&unix_socket_array_lock);
    struct unix_socket *out = *(struct unix_socket **)array_get(
        &unix_socket_array, id, sizeof(struct unix_socket *));
    spinlock_unlock(&unix_socket_array_lock);
    return out;
}

int unix_socket_add_array(struct unix_socket *sock)
{
    spinlock_lock(&unix_socket_array_lock);
    int retval =
        array_add(&unix_socket_array, &sock, sizeof(struct unix_socket *));
    spinlock_unlock(&unix_socket_array_lock);

    return retval;
}

struct unix_socket *unix_socket_alloc()
{
    struct unix_socket *out = malloc(sizeof(struct unix_socket));

    out->buffer = ARRAY_INIT();
    out->send_queue = WAIT_QUEUE_INIT();
    out->recv_queue = WAIT_QUEUE_INIT();

    out->is_closing = false;

    return out;
}

int unix_socket_recv(struct unix_socket *sock, void *buf, size_t len)
{

    if (WAIT_QUEUE_EMPTY(sock->send_queue))
    {
        wait_queue_wait(&sock->recv_queue);
        if (sock->is_closing)
        {
            return -ENOTSOCK;
        }
    }
    else
    {
        wait_queue_done(&sock->send_queue);
    }

    void *sent = array_pop(&sock->buffer, len);
    memcpy(buf, sent, len);
    free(sent);

    return 0;
}

int unix_socket_send(struct unix_socket *sock, void *buf, size_t len)
{
    array_add(&sock->buffer, buf, len);
    if (WAIT_QUEUE_EMPTY(sock->recv_queue))
    {
        wait_queue_wait(&sock->send_queue);
        if (sock->is_closing)
        {
            return -ENOTSOCK;
        }
    }
    else
    {
        wait_queue_done(&sock->recv_queue);
    }

    return 0;
}

int unix_socket_close(int socket_num)
{
    printf("closing:%d\n", socket_num);
    struct unix_socket *sock = unix_socket_lookup(socket_num);

    void *new_ptr = NULL;
    array_set(&unix_socket_array, socket_num, &new_ptr,
              sizeof(struct unix_socket *));

    void *should_be_null = unix_socket_lookup(socket_num);
    if (should_be_null != NULL)
    {
        printf("NOT NULL\n");
    }

    sock->is_closing = true;

    wait_queue_wake_all(&sock->recv_queue);
    wait_queue_wake_all(&sock->send_queue);
    /* TODO: we want to free the socket, but need to wait for everyone waiting
     * on the queue to be done */
    /* this could cause issues */
    /* array_free(&sock->buffer); */
    /* free(sock); */

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

SYSCALL_DEFINE3(SYS_send, int, int, socket, void *, buf, size_t, len)
{
    struct unix_socket *s = unix_socket_lookup(socket);
    if (s == NULL)
    {
        return -ENOTSOCK;
    }
    return unix_socket_send(s, (void *)buf, len);
}

SYSCALL_DEFINE3(SYS_recv, int, int, socket, void *, buf, size_t, len)
{
    struct unix_socket *s = unix_socket_lookup(socket);
    if (s == NULL)
    {
        return -ENOTSOCK;
    }
    return unix_socket_recv(s, (void *)buf, len);
}

SYSCALL_DEFINE0(SYS_create_socket, int)
{
    struct unix_socket *s = unix_socket_alloc();
    return unix_socket_add_array(s);
}

SYSCALL_DEFINE1(SYS_close_socket, int, int, socket)
{
    return unix_socket_close(socket);
}

/* SYSCALL_DEFINE0(socket_create, SYS_create_socket) */
/* SYSCALL_DEFINE3(syscall_socket_send, SYS_send) */
/* SYSCALL_DEFINE3(syscall_socket_recv, SYS_recv) */
