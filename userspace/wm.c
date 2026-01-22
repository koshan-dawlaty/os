#include "sched.h"
#include "wm_lib.h"
#include <fcntl.h>
#include <init.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

typedef uintptr_t ptr;
extern int syscall_wrapper_asm(ptr ax, ptr bx, ptr cx, ptr dx, ptr si, ptr di);

struct framebuffer *framebuffer_alloc()
{
    struct framebuffer *out = NULL;
    syscall_wrapper_asm(SYS_get_framebuffer, (ptr)&out, 0, 0, 0, 0);

    return out;
}

int socket_create()
{
    return syscall_wrapper_asm(SYS_create_socket, 0, 0, 0, 0, 0);
}

int send(int socket, void *buf, size_t len)
{
    return syscall_wrapper_asm(SYS_send, socket, (ptr)buf, len, 0, 0);
}

int recv(int socket, void *buf, size_t len)
{
    return syscall_wrapper_asm(SYS_recv, socket, (ptr)buf, len, 0, 0);
}

int clone(ptr ip)
{
    return syscall_wrapper_asm(SYS_clone, ip, 0, 0, 0, 0);
}

struct window *new_window(size_t x, size_t y, size_t w, size_t h)
{
    struct window *out = malloc(sizeof(struct window));
    out->x = x;
    out->y = y;
    out->w = w;
    out->h = h;

    /* TODO: this assumes 3 bytes per pixel */

    out->buffer = malloc(w * h * 4);

    return out;
}

int shared_mem_alloc(void **addr, size_t len)
{
    return syscall_wrapper_asm(SYS_shared_mem_alloc, (ptr)addr, len, 0, 0, 0);
}

int shared_mem_get(int id, void **p)
{
    return syscall_wrapper_asm(SYS_shared_mem_get, id, (ptr)p, 0, 0, 0);
}

void draw_window(struct framebuffer *b, struct window *win)
{

    for (size_t y = 0; y < win->h; y++)
    {
        /* TODO: assumes 3 bytes per pixel */
        /* printf("%p ", win->buffer + (y * win->w)); */
        memcpy(b->buffer +
                   (((y + win->y) * b->pitch) + (win->x * b->pixel_width)),
               win->buffer + (y * win->w * 4), win->w * 4);
    }
}

struct framebuffer *buffer;

int current_client = 1;

size_t x = 10;

void process_connection()
{
    int socket;
    socket = current_client;

    printf("client socket:%d\n", socket);

    struct wm_inital win_info;
    recv(socket, &win_info, sizeof(struct wm_inital));

    size_t memory_len = win_info.height * win_info.width * 4;
    void *client_buf;
    int mem_id = shared_mem_alloc(&client_buf, memory_len);

    /* printf("server buf:%p\n", client_buf); */

    /* printf("server:memory:%d\n", mem_id); */
    struct wm_second *second_message = malloc(sizeof(struct wm_second));
    second_message->mem_id = mem_id;

    send(socket, second_message, sizeof(struct wm_second));

    struct window *w = new_window(200 + x, x, win_info.width, win_info.height);
    x += 100;

    int code;
    recv(socket, &code, sizeof(int));
    while (code == 1)
    {

        for (size_t y = 0; y < w->h; y++)
        {
            size_t offset = y * w->w * 4;
            /* memcpy(w->buffer + offset, client_buf + offset, w->w * 3); */

            memcpy(w->buffer + offset, client_buf + offset, w->w * 4);
            /* memcpy(w->buffer + offset, client_buf + offset, w->w); */
        }

        draw_window(buffer, w);

        recv(socket, &code, sizeof(int));
    }

    printf("%p\n", memory_len);
}

void thread1()
{
    printf("thread\n");

    while (1)
    {
        sched_yield();
    }

    exit(0);
}

int main()
{
    init_libc();

    /* printf("window manager\n"); */
    /* clone((ptr)thread1); */

    buffer = framebuffer_alloc();
    int socket = socket_create();
    for (;;)
    {

        int client_sock = socket_create();
        send(socket, &client_sock, sizeof(int));
        current_client = client_sock;

        clone((ptr)(process_connection));
    }
    /* process_connection(client_sock); */

    exit(0);
}
