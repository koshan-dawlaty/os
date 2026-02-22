#include "sys/types.h"
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

struct wm_connection
{
    int client_socket;
    int input_socket;
    void *buffer;
};

struct wm_connection *wm_connection_alloc(int client_socket, void *client_buf)
{
    struct wm_connection *conn = malloc(sizeof(struct wm_connection));

    conn->client_socket = client_socket;
    conn->input_socket = socket_create();
    conn->buffer = client_buf;

    return conn;
}

struct window_queue_entry
{
    struct window *win;
    struct wm_connection *conn;
    struct list2 list;
};

struct window_queue_entry *focused = NULL;

char current_keyboard_char;

struct list2 *window_queue;

struct window *new_window(size_t x, size_t y, size_t w, size_t h)
{
    struct window *out = malloc(sizeof(struct window));
    out->x = x;
    out->y = y;
    out->w = w;
    out->h = h;

    out->buffer = malloc(w * h * 4);

    return out;
}

struct framebuffer *buffer;

void draw_window(struct framebuffer *b, struct window *win)
{

    for (size_t y = 0; y < win->h - 1; y++)
    {
        memcpy(b->buffer +
                   (((y + win->y) * b->pitch) + (win->x * b->pixel_width)),
               win->buffer + (y * win->w * 4), win->w * 4);
    }
}

void draw_windows(struct framebuffer *b)
{

    struct list2 *pos;
    list2_for_each(pos, window_queue)
    {
        struct window_queue_entry *ent =
            LIST_ENTRY(pos, struct window_queue_entry, list);
        draw_window(b, ent->win);
    }
}

int current_client = 1;

struct window *mouse;

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

    struct wm_connection *conn = wm_connection_alloc(socket, client_buf);

    printf("%d, %d\n", win_info.width, win_info.height);
    struct window *w = new_window(400, x, win_info.width, win_info.height);
    x += 175;

    struct window_queue_entry *ent = malloc(sizeof(struct window_queue_entry));
    ent->list = LIST2_INIT();
    ent->win = w;
    ent->conn = conn;

    if (window_queue == NULL)
    {
        window_queue = &ent->list;
    }
    else
    {
        list2_add_front(&ent->list, &window_queue);
    }

    focused = ent;

    int code;
    recv(socket, &code, sizeof(int));
    while (true)
    {
        /* printf("got code:%d\n", code); */
        switch (code)
        {
        case WM_RES_RENDER:
        {

            for (size_t y = 0; y < w->h - 1; y++)
            {
                size_t offset = y * w->w * 4;

                if (w->h == 100)
                {
                    if (*(u32 *)(client_buf + offset) != 0)
                    {
                        /* printf("TEST\n"); */
                    }
                }

                memcpy(w->buffer + offset, client_buf + offset, w->w * 4);
            }

            draw_windows(buffer);

            int ack_code = 3;
            send(socket, &ack_code, sizeof(int));

            /* draw_window(buffer, w); */
            break;
        }

        case WM_RES_KEY:
        {
            char input;
            /* printf("waiting for input\n"); */
            recv(conn->input_socket, &input, sizeof(char));
            printf("%c", input);
            send(socket, &input, sizeof(char));
            /* printf("sent it\n"); */
            break;
        }
        }
        /* printf("waiting for next code\n"); */
        recv(socket, &code, sizeof(int));
    }
}

void keyboard_thread()
{

    while (true)
    {
        char c = getchar();

        if (focused != NULL)
        {
            struct wm_connection *conn = focused->conn;
            current_keyboard_char = c;

            send(conn->input_socket, &c, sizeof(char));
        }
    }
}

void mouse_thread()
{

    int mouse_fd = open("/mouse", O_RDONLY);
    while (true)
    {
        struct mouse_event *ev = malloc(sizeof(struct mouse_event));
        int size = read(mouse_fd, ev, sizeof(struct mouse_event));

        if (size != sizeof(struct mouse_event))
        {
            /* printf("reading from the mouse returned invalid size!\n"); */
        }

        if (ev->x_change < 0)
        {
        }

        mouse->x += ev->x_change;
        mouse->y += ev->y_change;

        if ((int)mouse->x < 0)
        {
            mouse->x = 0;
        }

        if ((int)mouse->y < 0)
        {
            mouse->y = 0;
        }

        if (ev->buttons.left_click)
        {

            /* TODO: this should be its own function that traverses the list
             * backwards */
            struct list2 *pos;
            struct list2 *prev = NULL;
            list2_for_each(pos, window_queue)
            {
                prev = pos;
            }

            for (pos = prev; pos != NULL; pos = pos->prev)
            {

                struct window_queue_entry *queue_ent =
                    LIST_ENTRY(pos, struct window_queue_entry, list);
                struct window *win = queue_ent->win;

                if (mouse->x > win->x && mouse->x < win->x + win->w &&
                    mouse->y > win->y && mouse->y < win->y + win->h)
                {
                    focused = queue_ent;
                    break;
                }
            }

            if (focused != NULL)
            {
                memset(focused->win->buffer, 127, focused->win->w * 4 * 10);
                /* memset(focused->win->buffer, 0, */
                /*        focused->win->w * focused->win->h * 4); */
            }
        }

        /* printf("%d, %d\n", mouse->x, mouse->y); */

        /* draw_window(buffer, mouse); */
        draw_windows(buffer);

        free(ev);
    }
}

void thread1()
{
    /* printf("thread\n"); */

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

    mouse = new_window(0, 0, 10, 10);

    /* memset(mouse->buffer, 255, 10 * 10 * 4); */

    /* this might be bad because this is a stack allocation */
    struct window_queue_entry mouse_ent = {.win = mouse, .list = LIST2_INIT()};
    window_queue = &mouse_ent.list;

    clone((ptr)mouse_thread);
    clone((ptr)keyboard_thread);

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
