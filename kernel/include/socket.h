#ifndef SOCKET_H
#define SOCKET_H

#include <lib/array.h>
#include <stddef.h>
#include <wait_queue.h>

enum socket_state
{
    SOCKET_LISTEN,
    SOCKET_CONNECTED,
    SOCKET_DEFAULT,
};

struct unix_socket
{
    enum socket_state state;
    union
    {
        struct
        {
            size_t backlog;
            struct unix_socket *waiting;
        } listener;
        struct
        {
            struct list2 list;
        } connected;
    };

    struct array buffer;          /* buffer of things that has been sent */
    struct wait_queue send_queue; /* queue that is waiting on send() */
    struct wait_queue recv_queue; /* queue that is waiting on recv() */
};

struct unix_socket *unix_socket_alloc();

int unix_socket_recv(struct unix_socket *sock, void *buf, size_t len);
int unix_socket_send(struct unix_socket *sock, void *buf, size_t len);

/* TODO: these are just here so userspace works, they should be removed and
 * replaced with the actual api */
int unix_socket_add_array(struct unix_socket *sock);
struct unix_socket *unix_socket_lookup(int id);

#endif
