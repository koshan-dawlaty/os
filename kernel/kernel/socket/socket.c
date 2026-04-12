#include "fcntl.h"
#include "kernel/userspace.h"
#include "lib/assert.h"
#include "panic.h"
#include "scheduler.h"
#include "spinlock.h"
#include "sys/types.h"
#include "unistd.h"
#include <err_ptr.h>
#include <errno.h>
#include <kernel/syscall.h>
#include <lib/array.h>
#include <lib/list.h>
#include <lib/log.h>
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

struct unix_socket *unix_socket_alloc()
{
    struct unix_socket *out = malloc(sizeof(struct unix_socket));

    out->buffer = ARRAY_INIT();
    wait_queue_init(&out->send_queue);
    wait_queue_init(&out->recv_queue);

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

int unix_socket_send(struct unix_socket *sock, const void *buf, size_t len)
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

int unix_socket_close(struct file *f)
{
    logd("socket closed");

    struct unix_socket *sock = f->file_private;

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

static inline int unix_socket_fs_read(struct file *f, char *buf, size_t n,
                                      size_t off)
{
    struct unix_socket *socket = f->file_private;
    return unix_socket_recv(socket, buf, n);
}
static inline int unix_socket_fs_write(struct file *f, const char *buf,
                                       size_t n, size_t off)
{
    struct unix_socket *socket = f->file_private;
    return unix_socket_send(socket, buf, n);
}

struct file_operations socket_operations = {
    .read = unix_socket_fs_read,
    .write = unix_socket_fs_write,
    .close = unix_socket_close,
};

/*
 * creates a socket that an anonymous file (not associated with a dentry)
 */
struct file *unix_socket_create()
{
    /* TODO: dont use new_file for this, as it takes a struct dentry as a
     * parameter */
    struct file *out = new_file(NULL, O_RDWR);
    out->file_private = unix_socket_alloc();
    out->operations = socket_operations;

    return out;
}

/*
 * creates a socket that is associated with a dentry
 */
struct file *unix_socket_create_fs(const char *path)
{
    /* FIXME: this should not make a new dentry like this */
    struct pathname_lookup_flags flags = {
        .create = true,
        .type = DENTRY_SOCKET, /* TODO: dentry type here is bad */
    };
    struct unix_socket *socket = unix_socket_alloc();
    struct dentry *entry = lookup_path(path, flags);

    entry->socket = socket;
    socket->connection.task = get_current_task();
    wait_queue_init(&socket->connection.accept_queue);
    mutex_init(&socket->connection.lock);
    socket->connection.buffer = ARRAY_INIT();

    struct file *file = new_file(entry, O_RDWR);
    file->operations = socket_operations;
    file->file_private = socket;

    return file;
}

/*
 * establishes a connection with a filesystem socket
 * returns the new socket for the client, not the server
 */
int unix_socket_connect(struct dentry *entry)
{
    struct unix_socket *socket = entry->socket;

    struct file *new_sock = unix_socket_create();

    struct task *current = get_current_task();
    int fd = fdlist_add(current->descriptors, new_sock);
    struct task *other_task = socket->connection.task;
    int other_fd = fdlist_add(other_task->descriptors, new_sock);
    mutex_lock(&socket->connection.lock);
    socket->connection.current_connection = other_fd;
    mutex_unlock(&socket->connection.lock);
    array_push(&socket->connection.buffer, &other_fd, sizeof(int));
    if (!WAIT_QUEUE_EMPTY(socket->connection.accept_queue))
    {
        wait_queue_done(&socket->connection.accept_queue);
    }

    return fd;
}

int unix_socket_accept(struct dentry *entry)
{
    struct unix_socket *socket = entry->socket;

    if (ARRAY_LEN(socket->connection.buffer, sizeof(int)) == 0)
    {
        wait_queue_wait(&socket->connection.accept_queue);
    }
    int *out_ptr = array_pop(&socket->connection.buffer, sizeof(int));

    return *out_ptr;
}

struct dentry *dentry_socket_alloc(const char *name, struct dentry *parent,
                                   struct unix_socket *socket)
{
    struct dentry *out = malloc(sizeof(struct dentry));
    out->name = (char *)name;
    out->parent = parent;
    out->type = DENTRY_SOCKET;
    out->socket = socket;

    return out;
}

SYSCALL_DEFINE1(SYS_accept, int, int, fd)
{
    struct file *f = lookup_descriptor(fd);
    struct dentry *entry = f->entry;
    ASSERT(entry->type == DENTRY_SOCKET,
           "accept needs to be called on a socket dentry");

    int out = unix_socket_accept(entry);

    return out;
}

SYSCALL_DEFINE1(SYS_connect, int, int, fd)
{
    struct file *f = lookup_descriptor(fd);
    struct dentry *entry = f->entry;
    ASSERT(entry->type == DENTRY_SOCKET,
           "accept needs to be called on a socket dentry");

    int out = unix_socket_connect(entry);

    return out;
}

SYSCALL_DEFINE0(SYS_create_socket, int)
{
    struct file *f = unix_socket_create();
    int fd = fdlist_add(get_current_task()->descriptors, f);

    return fd;
}

SYSCALL_DEFINE1(SYS_create_socket_fs, int, const char *, pathname)
{
    const char *path = get_user_pathname(pathname);
    if (IS_ERR(path))
    {
        return GET_ERR(path);
    }
    struct file *f = unix_socket_create_fs(path);

    int fd = fdlist_add(get_current_task()->descriptors, f);

    return fd;
}

/* SYSCALL_DEFINE0(socket_create, SYS_create_socket) */
/* SYSCALL_DEFINE3(syscall_socket_send, SYS_send) */
/* SYSCALL_DEFINE3(syscall_socket_recv, SYS_recv) */
