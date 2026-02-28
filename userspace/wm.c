#include "sys/types.h"
#include "wm_lib.h"
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/list.h>
#include <lib/rect.h>
#include <lib/utils.h>
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

static inline size_t get_framebuffer_y(struct framebuffer *f,
                                       struct window *win, size_t y)
{
    return ((y + win->r.y) * f->pitch);
}

struct window *mouse;

struct window_queue_entry
{
    struct window *win;
    struct wm_connection *conn;
    struct list2 list;
};

struct list2 *window_queue;

void win_draw_rect(struct window *win, struct rect area, struct framebuffer *fb)
{

    size_t x1 = MAX(win->r.x, area.x);
    size_t y1 = MAX(win->r.y, area.y);

    size_t x2 = MIN(win->r.x + win->r.w, area.x + area.w);
    size_t y2 = MIN(win->r.y + win->r.h, area.y + area.h);

    size_t draw_height = y2 - y1;
    size_t draw_width = x2 - x1;
    for (size_t y = 0; y < draw_height; y++)
    {
        size_t fb_y = y1 + y;
        size_t win_y = (fb_y - win->r.y);

        size_t fb_offset = (fb_y * fb->pitch) + (x1 * fb->pixel_width);

        size_t win_offset = (win_y * win->r.w + (x1 - win->r.x)) * 4;

        memcpy(fb->buffer + fb_offset, win->buffer + win_offset,
               draw_width * 4);
    }
}

/*
 * @win the lowest window that needs to be updated
 */
void update_area_up(struct window_queue_entry *ent, struct rect area,
                    struct framebuffer *fb)
{

    struct list2 *pos;
    /* list2_for_each(pos, &ent->list) */
    list2_for_each(pos, window_queue)
    {
        struct window_queue_entry *ent =
            containerof(pos, struct window_queue_entry, list);

        if (rect_intersect(ent->win->r, area))
        {
            win_draw_rect(ent->win, area, fb);
        }
    }
}

struct window_queue_entry *focused = NULL;

char current_keyboard_char;

struct window *new_window(size_t x, size_t y, size_t w, size_t h)
{
    struct window *out = malloc(sizeof(struct window));
    out->r = (struct rect){x, y, w, h};

    out->buffer = malloc(w * h * 4);

    return out;
}

struct framebuffer *buffer;

void draw_window(struct framebuffer *b, struct window *win)
{

    for (int y = 0; y < win->r.h - 1; y++)
    {
        memcpy(b->buffer +
                   (((y + win->r.y) * b->pitch) + (win->r.x * b->pixel_width)),
               win->buffer + (y * win->r.w * 4), win->r.w * 4);
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
    x += win_info.height - 10;

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

            for (int y = 0; y < w->r.h - 1; y++)
            {
                size_t offset = y * w->r.w * 4;

                if (w->r.h == 100)
                {
                    if (*(u32 *)(client_buf + offset) != 0)
                    {
                        /* printf("TEST\n"); */
                    }
                }

                memcpy(w->buffer + offset, client_buf + offset, w->r.w * 4);
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
            /* printf("%c", input); */
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

int drag_count = 0;

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

        mouse->r.x += ev->x_change;
        mouse->r.y += ev->y_change;

        if ((int)mouse->r.x < 0)
        {
            mouse->r.x = 0;
        }

        if ((int)mouse->r.y < 0)
        {
            mouse->r.y = 0;
        }

        draw_window(buffer, mouse);

        if (ev->buttons.left_click && drag_count < 3)
        {

            drag_count++;

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

                if (mouse->r.x > win->r.x && mouse->r.x < win->r.x + win->r.w &&
                    mouse->r.y > win->r.y && mouse->r.y < win->r.y + win->r.h)
                {
                    focused = queue_ent;
                    break;
                }
            }

            if (focused != NULL)
            {
                /* memset(focused->win->buffer, 127, focused->win->w * 4 * 10);
                 */
                /* memset(focused->win->buffer, 0, */
                /*        focused->win->w * focused->win->h * 4); */
            }
        }
        else if (ev->buttons.left_click)
        {
            if (focused != NULL)
            {
                struct rect old_area = focused->win->r;

                focused->win->r.x += ev->x_change;
                focused->win->r.y += ev->y_change;

                struct rect move_area = {
                    .x = focused->win->r.x,
                    .y = focused->win->r.y,
                    .w = abs(ev->x_change + focused->win->r.w),
                    .h = abs(ev->y_change + focused->win->r.h),
                };
                /* printf("%d ", move_area.h); */
                /* printf("%d, %d\n", old_area.x, focused->win->r.x); */

                /* update_area_up(focused, move_area, buffer); */
                update_area_up(focused, focused->win->r, buffer);
                /* update_area_up(focused, old_area, buffer); */
                update_area_up(focused,
                               (struct rect){.x = 0,
                                             .y = 0,
                                             .h = buffer->height,
                                             .w = buffer->width},
                               buffer);
            }
        }
        else
        {
            drag_count = 0;
        }

        /* printf("%d, %d\n", mouse->x, mouse->y); */

        /* draw_window(buffer, mouse); */
        /* draw_windows(buffer); */

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

    size_t mouse_w = 10;
    size_t mouse_h = 10;
    mouse = new_window(0, 0, mouse_w, mouse_h);

    for (size_t y = 0; y < mouse_h; y++)
    {
        for (size_t x = 0; x < mouse_w; x++)
        {
            u32 val = x * x * x * y * y * y * y;
            *(u32 *)(mouse->buffer + (x * 4) + (y * mouse_w * 4)) = val;
        }
    }
    /* memset(mouse->buffer, 255, 10 * 10 * 4); */

    /* this might be bad because this is a stack allocation */
    struct window_queue_entry mouse_ent = {.win = mouse, .list = LIST2_INIT()};
    window_queue = &mouse_ent.list;

    clone((ptr)mouse_thread);
    clone((ptr)keyboard_thread);

    buffer = framebuffer_alloc();
    int socket = socket_create();

    /* execv("/elf_test", (char *[]){"elf_test", NULL}); */
    execv("/elf_test", (char *[]){"elf_test", NULL});
    execv("/elf_test", (char *[]){"elf_test", NULL});
    execv("imgview", (char *[]){"imgview", "osdev.tga", NULL});

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
