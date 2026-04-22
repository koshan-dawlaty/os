#ifndef SOCKET_H
#define SOCKET_H

#include "spinlock.h"
#include <lib/array.h>
#include <stddef.h>
#include <vfs.h>
#include <wait_queue.h>

struct unix_socket
{
    bool is_closing;

    union
    {
        struct
        {
            struct task *task;
            struct wait_queue accept_queue;
            struct array buffer;
            struct mutex lock;

            int current_connection; /* TODO: this is used to tell the socket
                                        what the new socket is after it has
                                        woken up from accept(), but it should be
                                        message passing or something */
        } connection; /* used if the socket exists on the filesystem */
    };

    struct array buffer;          /* buffer of things that has been sent */
    struct wait_queue send_queue; /* queue that is waiting on send() */
    struct wait_queue recv_queue; /* queue that is waiting on recv() */
};

struct unix_socket *unix_socket_alloc();

int unix_socket_recv(struct unix_socket *sock, void *buf, size_t len);
int unix_socket_send(struct unix_socket *sock, const void *buf, size_t len);

/* TODO: these are just here so userspace works, they should be removed and
 * replaced with the actual api */
int unix_socket_add_array(struct unix_socket *sock);
struct unix_socket *unix_socket_lookup(int id);

/* FIXME: this is very stupid and only called by pathname lookup */
struct dentry *dentry_socket_alloc(const char *name, struct dentry *parent,
                                   struct unix_socket *socket);
extern struct file_operations socket_operations;

/*
 * creates a socket that an anonymous file (not associated with a dentry)
 */
struct file *unix_socket_create();

#endif
