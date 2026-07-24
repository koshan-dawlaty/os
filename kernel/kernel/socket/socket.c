#include "fcntl.h"
#include "kernel/userspace.h"
#include "lib/assert.h"
#include "lib/refcount.h"
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

    /* logd("recv:%s", sock->buffer.data); */
    logd("%d bytes in buffer, %d requested", sock->buffer.len, len);

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
    logd("closing socket");

    struct unix_socket *sock = f->file_private;
    sock->is_closing = true;

    struct list *recv_queue = &sock->recv_queue.queue.head;
    struct task *t = containerof(recv_queue->next, struct task, list);

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

bool unix_socket_poll(struct file *f, struct task_queue **queue)
{
    struct unix_socket *socket = f->file_private;
    /* TODO: return mask when poll api is changed */
    return !WAIT_QUEUE_EMPTY(socket->recv_queue);
}

void socket_inode_write(struct inode *inode, const u8 *buf, size_t n,
                        size_t off)
{
    struct unix_socket *socket = inode->fs_private;
    unix_socket_send(socket, buf, n);
}

void socket_inode_read(struct inode *inode, u8 *buf, size_t n, size_t off)
{
    struct unix_socket *socket = inode->fs_private;
    unix_socket_recv(socket, buf, n);
}

struct fs_type socket_fs = {

    .is_permanent = true,
    .inode_functions = {.read_inode = socket_inode_read,
                        .write_inode = socket_inode_write},
    .name = "socketfs",
    .file_operations = {.read = unix_socket_fs_read,
                        .write = unix_socket_fs_write,
                        .close = unix_socket_close,
                        .poll = file_default_poll}};

struct superblock socketfs_superblock =
    SUPERBLOCK_INIT(0, NULL, NULL, &socket_fs);

struct file *unix_socket_create()
{
    /* TODO: dont use new_file for this, as it takes a struct dentry as a
     * parameter */
    struct file *out = new_file(NULL, O_RDWR);
    out->file_private = unix_socket_alloc();
    out->operations = socket_fs.file_operations;

    return out;
}

struct inode *socket_inode_alloc(struct unix_socket *sock)
{
    struct superblock *sb = &socketfs_superblock;

    struct inode *out = new_inode(0, 0, sock, sb);
    return out;
}

/*
 * creates a socket that is associated with a dentry
 */
struct file *unix_socket_create_fs(const char *path)
{
    struct unix_socket *socket = unix_socket_alloc();

    struct pathname_lookup_flags flags = {.create = true,
                                          .type = DENTRY_FILE,
                                          {.node = socket_inode_alloc(socket)}};

    struct dentry *entry = lookup_path(path, flags);

    /* entry->socket = socket; */
    socket->connection.task = get_current_task();
    wait_queue_init(&socket->connection.accept_queue);
    mutex_init(&socket->connection.lock);
    socket->connection.buffer = ARRAY_INIT();

    struct file *file = new_file(entry, O_RDWR);

    return file;
}

/*
 * establishes a connection with a filesystem socket
 * returns the new socket for the client, not the server
 */
int unix_socket_connect(struct dentry *entry)
{
    struct unix_socket *socket = entry->inode->fs_private;

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
    struct unix_socket *socket = entry->inode->fs_private;

    if (ARRAY_LEN(socket->connection.buffer, sizeof(int)) == 0)
    {
        wait_queue_wait(&socket->connection.accept_queue);
    }
    int *out_ptr = array_pop(&socket->connection.buffer, sizeof(int));

    return *out_ptr;
}

SYSCALL_DEFINE1(SYS_accept, int, int, fd)
{
    struct file *f = lookup_descriptor(fd);
    struct dentry *entry = f->entry;

    int out = unix_socket_accept(entry);

    return out;
}

SYSCALL_DEFINE1(SYS_connect, int, int, fd)
{
    struct file *f = lookup_descriptor(fd);
    struct dentry *entry = f->entry;

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
