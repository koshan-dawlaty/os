#include <kernel/syscall.h>
#include <lib/array.h>
#include <lib/list.h>
#include <sched.h>
#include <socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <task_queue.h>
#include <utils.h>
#include <vfs.h>
#include <wait_queue.h>

struct array unix_socket_array = ARRAY_INIT();

struct unix_socket *unix_socket_lookup(int id)
{
    printf("looking up socket:%d\n", id);
    return array_get(&unix_socket_array, id, sizeof(struct unix_socket));
}

int unix_socket_add_array(struct unix_socket *sock)
{
    int retval =
        array_add(&unix_socket_array, sock, sizeof(struct unix_socket));
    printf("created socket number:%d\n", retval);

    return retval;
}

struct unix_socket *unix_socket_alloc()
{
    struct unix_socket *out = malloc(sizeof(struct unix_socket));

    out->buffer = ARRAY_INIT();
    out->send_queue = WAIT_QUEUE_INIT();
    out->recv_queue = WAIT_QUEUE_INIT();

    return out;
}

int unix_socket_recv(struct unix_socket *sock, void *buf, size_t len)
{

    if (WAIT_QUEUE_EMPTY(sock->send_queue))
    {
        wait_queue_wait(&sock->recv_queue);
    }
    else
    {
        wait_queue_done(&sock->send_queue);
    }

    /* TODO: this should remove data and not go out of bounds */
    void *sent = array_pop(&sock->buffer, len);
    memcpy(buf, sent, len);

    return 0;
}

int unix_socket_send(struct unix_socket *sock, void *buf, size_t len)
{
    array_add(&sock->buffer, buf, len);
    if (WAIT_QUEUE_EMPTY(sock->recv_queue))
    {
        wait_queue_wait(&sock->send_queue);
    }
    else
    {
        wait_queue_done(&sock->recv_queue);
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

int socket_create()
{
    struct unix_socket *s = unix_socket_alloc();
    return unix_socket_add_array(s);
}

int syscall_socket_send(int socket, void *buf, size_t len)
{
    struct unix_socket *s = unix_socket_lookup(socket);
    return unix_socket_send(s, (void *)buf, len);
}

int syscall_socket_recv(int socket, void *buf, size_t len)
{
    struct unix_socket *s = unix_socket_lookup(socket);
    return unix_socket_recv(s, (void *)buf, len);
}

SYSCALL_DEFINE0(socket_create, SYS_create_socket)
SYSCALL_DEFINE3(syscall_socket_send, SYS_send)
SYSCALL_DEFINE3(syscall_socket_recv, SYS_recv)
