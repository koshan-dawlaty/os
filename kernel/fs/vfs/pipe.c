#include <fcntl.h>
#include <kernel_syscall.h>
#include <lib/array.h>
#include <lib/utils.h>
#include <scheduler.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <syscall.h>
#include <vfs.h>
#include <wait_queue.h>

struct pipe
{
    struct wait_queue queue;
    struct pipe *other;
    struct array buffer;
};

/* TODO: add a check if the pipe is the read or write side for read/write */
int pipe_read(struct file *file, char *buf, size_t n, size_t off)
{
    struct pipe *pipe = file->file_private;
    struct pipe *write = pipe->other;

    /* printf("len:%p\n", write->buffer.len); */
    if (write->buffer.len == 0)
    {
        wait_queue_wait(&write->queue);
    }

    /* returning only what is in the pipe (not always n bytes) is kinda weird,
     * but it is what the standard for linux does (at least from what i can
     * tell) */

    size_t read_size = MIN(n, write->buffer.len);

    void *result = array_pop(&write->buffer, read_size);
    memcpy(buf, result, read_size);
    free(result);

    return read_size;
}

int pipe_write(struct file *file, const char *buf, size_t n, size_t off)
{

    struct pipe *pipe = file->file_private;
    struct pipe *read = pipe->other;

    /* printf("buf:%s\n", buf); */
    array_add(&pipe->buffer, buf, n);

    if (!WAIT_QUEUE_EMPTY(pipe->queue))
    {
        wait_queue_done(&pipe->queue);
    }

    return n;
}

bool pipe_poll(struct file *f, struct wait_queue **queue)
{
    struct pipe *pipe = f->file_private;
    *queue = &pipe->queue;

    /* might only work for read side, need a way of determining which side is
     * which */
    return pipe->buffer.len == 0;
}

const struct file_operations pipe_operations = {
    .read = pipe_read, .write = pipe_write, .poll = pipe_poll};

int pipe_open(struct file **read_ptr, struct file **write_ptr)
{
    *read_ptr = new_file(NULL, O_RDONLY);
    *write_ptr = new_file(NULL, O_WRONLY);

    struct file *read = *read_ptr;
    struct file *write = *write_ptr;

    struct pipe *read_pipe = malloc(sizeof(struct pipe));

    wait_queue_init(&read_pipe->queue);
    read_pipe->buffer = ARRAY_INIT();
    read->file_private = read_pipe;
    read->operations = pipe_operations;

    struct pipe *write_pipe = malloc(sizeof(struct pipe));
    wait_queue_init(&write_pipe->queue);
    write_pipe->buffer = ARRAY_INIT();
    write->file_private = write_pipe;
    write->operations = pipe_operations;

    read_pipe->other = write_pipe;
    write_pipe->other = read_pipe;

    return 0;
}

int syscall_pipe(int fds[2])
{
    struct file *read, *write;
    pipe_open(&read, &write);

    fds[0] = fdlist_add(get_current_task()->descriptors, read);
    fds[1] = fdlist_add(get_current_task()->descriptors, write);

    return 0;
}

SYSCALL_DEFINE1(SYS_pipe, int, int *, fds)
{
    return syscall_pipe(fds);
}
