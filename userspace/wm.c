#include "sys/types.h"
#include "wm_lib.h"
#include <container_of.h>
#include <errno.h>
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
struct framebuffer *buffer;

void win_draw_rect(struct window *win, struct rect area, struct framebuffer *fb)
{

    size_t x1 = MAX(win->r.x, area.x);
    size_t y1 = MAX(win->r.y, area.y);

    size_t x2 = MIN(win->r.x + win->r.w, area.x + area.w);
    size_t y2 = MIN(win->r.y + win->r.h, area.y + area.h);

    size_t draw_height = y2 - y1;
    size_t draw_width = x2 - x1;

    if (x1 >= x2 || y1 >= y2)
    {
        return;
    }

    for (size_t y = 0; y < draw_height; y++)
    {
        size_t fb_y = y1 + y;
        size_t win_y = (fb_y - win->r.y);

        size_t fb_offset = (fb_y * fb->pitch) + (x1 * fb->pixel_width);

        size_t win_offset = (win_y * win->r.w + (x1 - win->r.x)) * 4;

        memcpy(fb->buffer + fb_offset, win->buffer + win_offset,
               draw_width * 4);
        /* memset(fb->buffer + fb_offset, 127, draw_width * 4); */
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
        struct window_queue_entry *ent2 =
            containerof(pos, struct window_queue_entry, list);

        /* if (ent2->win == mouse) */
        /* { */
        /*     return; */
        /* } */

        struct window *win = ent2->win;

        if (rect_intersect(win->r, area))
        {
            /* if (win != ent->win && win != mouse) */
            /* { */
            /*     /1* printf("intersect\n"); *1/ */
            /* } */
            win_draw_rect(ent2->win, area, fb);
        }
    }
}

#define WINDOW_DECORATION_ROWS 10
struct window_queue_entry *focused = NULL;

/* correctly applys window decorations to newly focused window */
void change_focus(struct window_queue_entry *new_focus)
{

    for (size_t y = 0; y < WINDOW_DECORATION_ROWS; y++)
    {
        for (int x = 0; x < new_focus->win->r.w; x++)
        {
            *(u32 *)(new_focus->win->buffer + y * new_focus->win->r.w * 4 +
                     (x * 4)) = 0xFF00FF;
        }
    }
    struct rect new_rect = new_focus->win->r;
    update_area_up(
        new_focus,
        RECT_INIT(new_rect.x, new_rect.y, new_rect.w, WINDOW_DECORATION_ROWS),
        buffer);

    for (size_t y = 0; y < WINDOW_DECORATION_ROWS; y++)
    {
        for (int x = 0; x < focused->win->r.w; x++)
        {
            *(u32 *)(focused->win->buffer + y * focused->win->r.w * 4 +
                     (x * 4)) = 0;
        }
    }

    struct rect prev_rect = focused->win->r;
    update_area_up(focused,
                   RECT_INIT(prev_rect.x, prev_rect.y, prev_rect.w,
                             WINDOW_DECORATION_ROWS),
                   buffer);

    focused = new_focus;
}

char current_keyboard_char;

struct window *new_window(size_t x, size_t y, size_t w, size_t h)
{
    struct window *out = malloc(sizeof(struct window));
    out->r = (struct rect){x, y, w, h};

    out->buffer = malloc(w * h * 4);

    return out;
}

struct window *new_client_window(size_t x, size_t y, size_t w, size_t h)
{
    struct window *out = malloc(sizeof(struct window));
    out->r = (struct rect){x, y, w, h + WINDOW_DECORATION_ROWS};

    /* allocate extra space for decorations */
    out->buffer = malloc(w * (out->r.h) * 4);

    return out;
}

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
    struct window *w =
        new_client_window(400, x, win_info.width, win_info.height);
    x += win_info.height + 20;

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

            for (int y = 0; y < (w->r.h - WINDOW_DECORATION_ROWS) - 1; y++)
            {
                size_t server_offset =
                    (y + WINDOW_DECORATION_ROWS) * w->r.w * 4;
                size_t client_offset = y * w->r.w * 4;

                memcpy(w->buffer + server_offset, client_buf + client_offset,
                       w->r.w * 4);
            }

            draw_windows(buffer);

            int ack_code = 3;
            int retval = send(socket, &ack_code, sizeof(int));
            if (retval == -ENOTSOCK)
            {
                goto disconnect;
            }

            /* draw_window(buffer, w); */
            break;
        }

        case WM_RES_KEY:
        {
            char input;
            /* printf("waiting for input\n"); */
            int retval = recv(conn->input_socket, &input, sizeof(char));
            if (retval == -ENOTSOCK)
            {
                goto disconnect;
            }
            /* printf("%c", input); */
            retval = send(socket, &input, sizeof(char));
            if (retval == -ENOTSOCK)
            {
                goto disconnect;
            }
            /* printf("sent it\n"); */
            break;
        }
        }
        /* printf("waiting for next code\n"); */
        int retval = recv(socket, &code, sizeof(int));
        if (retval == -ENOTSOCK)
        {
            goto disconnect;
        }
    }

disconnect:;
    printf("disconnect!\n");

    list2_remove(&ent->list, &window_queue);

    exit(1);
}

void keyboard_thread()
{

    while (true)
    {
        char c = getchar();

        if (c == '\t')
        {
            struct list2 *next = focused->list.next;
            if (next == NULL)
            {
                next = window_queue;
            }
            struct window_queue_entry *ent =
                containerof(next, struct window_queue_entry, list);

            if (ent->win == mouse)
            {
                ent =
                    containerof(window_queue, struct window_queue_entry, list);
            }
            change_focus(ent);
        }
        else
        {

            if (focused != NULL)
            {
                struct wm_connection *conn = focused->conn;
                current_keyboard_char = c;

                send(conn->input_socket, &c, sizeof(char));
            }
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

        struct rect old_mouse = mouse->r;
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
        /* TODO: dont pass null */
        update_area_up(NULL, mouse->r, buffer);
        update_area_up(NULL, old_mouse, buffer);

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

            struct window_queue_entry *new_focus = NULL;

            for (pos = prev; pos != NULL; pos = pos->prev)
            {

                struct window_queue_entry *queue_ent =
                    LIST_ENTRY(pos, struct window_queue_entry, list);
                struct window *win = queue_ent->win;

                if (mouse->r.x > win->r.x && mouse->r.x < win->r.x + win->r.w &&
                    mouse->r.y > win->r.y && mouse->r.y < win->r.y + win->r.h)
                {
                    new_focus = queue_ent;
                    break;
                }
            }

            if (new_focus != NULL)
            {
                /* memset(focused->win->buffer, 127, focused->win->w * 4 * 10);
                 */

                /* memset(focused->win->buffer, 0, */
                /*        focused->win->w * focused->win->h *
                 * 4); */

                change_focus(new_focus);
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

                /* update_area_up(focused, move_area, buffer); */
                update_area_up(focused, focused->win->r, buffer);
                update_area_up(focused, old_area, buffer);
                /* update_area_up(focused, */
                /*                (struct rect){.x = 0, */
                /*                              .y = 0, */
                /*                              .h = buffer->height, */
                /*                              .w = buffer->width}, */
                /*                buffer); */
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
    execv("imgview", (char *[]){"imgview", "osdev.tga", NULL});
    execv("/elf_test", (char *[]){"elf_test", NULL});
    execv("/elf_test", (char *[]){"elf_test", NULL});

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
