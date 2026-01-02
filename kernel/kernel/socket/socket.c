#include <lib/list.h>
#include <task_queue.h>

enum socket_state
{
    SOCKET_LISTEN,
    SOCKET_CONNECTED,
    SOCKET_DEFAULT,
};

struct socket
{
    enum socket_state state;
    union
    {
        struct
        {
            size_t backlog;
            struct socket *waiting;
        } listener;
        struct
        {
            struct list2 list;
        } connected;
        struct
        {
        } default_state;
    };

    union
    {
        struct
        {
            struct task *current_task;
        } unix;
    } protocol;
};

int socket_listen(struct socket *sock, size_t backlog)
{

    if (sock->state != SOCKET_DEFAULT)
    {
        return -1; /* TODO: real error code */
    }
    sock->state = SOCKET_LISTEN;
    sock->listener.backlog = backlog;

    return 0;
}

int socket_accept(struct socket *sock, struct socket *client)
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
        next, struct socket,
        connected.list); /* this might be incorrect way to use containerof() */

    return 0;
}

int socket_connect(struct socket *client, struct socket *server)
{
    if (client->state != SOCKET_DEFAULT)
    {
        return -1; /* TODO: real error code */
    }
    client->state = SOCKET_CONNECTED;
    client->connected.list = LIST2_INIT();
    struct socket *waiting_queue_head = server->listener.waiting;
    if (waiting_queue_head == NULL)
    {
        server->listener.waiting = client;
        return 0;
    }

    client->connected.list.next = &waiting_queue_head->connected.list;
    waiting_queue_head->connected.list.prev = &client->connected.list;

    return 0;
}
