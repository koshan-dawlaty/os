#include "wm_lib.h"
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/list.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <userspace/mouse.h>

struct array windows = ARRAY_INIT();

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

struct framebuffer *buffer;

void draw_window(struct framebuffer *b, struct window *win)
{

    for (size_t y = 0; y < win->h - 1; y++)
    {
        /* TODO: assumes 3 bytes per pixel */
        /* printf("%p ", win->buffer + (y * win->w)); */
        memcpy(b->buffer +
                   (((y + win->y) * b->pitch) + (win->x * b->pixel_width)),
               win->buffer + (y * win->w * 4), win->w * 4);
    }
}

void draw_windows(struct framebuffer *b)
{
    size_t len = ARRAY_LEN(windows, sizeof(struct window));

    for (size_t i = 0; i < len; i++)
    {
        /* printf("i:%d ", i); */
        struct window *win = array_get(&windows, i, sizeof(struct window));
        draw_window(b, win);
    }
}

int current_client = 1;

size_t x = 10;

void process_connection()
{
    int socket;
    socket = current_client;

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

    struct window *w = new_window(400 + x, x, win_info.width, win_info.height);
    x += 50;

    size_t index = array_add(&windows, w, sizeof(struct window));

    int code;
    recv(socket, &code, sizeof(int));
    while (true)
    {

        if (code == WM_RES_RENDER)
        {

            for (size_t y = 0; y < w->h; y++)
            {
                size_t offset = y * w->w * 4;

                memcpy(w->buffer + offset, client_buf + offset, w->w * 4);
            }

            draw_windows(buffer);
            /* draw_window(buffer, w); */
        }

        recv(socket, &code, sizeof(int));
    }
}

void mouse_thread()
{
    int mouse_fd = open("/mouse", O_RDONLY);
    while (true)
    {
        struct mouse_event *ev = malloc(sizeof(struct mouse_event));
        int size = read(mouse_fd, ev, sizeof(struct mouse_event));

        free(ev);
    }
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

    clone((ptr)mouse_thread);

    buffer = framebuffer_alloc();
    int socket = socket_create();
    while (true)
    {

        int client_sock = socket_create();
        send(socket, &client_sock, sizeof(int));
        current_client = client_sock;

        clone((ptr)(process_connection));
    }
    /* process_connection(client_sock); */

    exit(0);
}
