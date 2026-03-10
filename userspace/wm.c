#include "sys/types.h"
#include "wm_lib.h"
#include <container_of.h>
#include <errno.h>
#include <extension.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/framebuffer.h>
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
    return ((y + WIN_Y(win)) * f->pitch);
}

struct window *mouse;

struct window_queue_entry
{
    struct window *win;
    struct wm_connection *conn;
    struct list2 list;

    /* FIXME: stupid, should use O_NONBLOCK globally or just find a better way
     * of doing this */
    struct
    {
        char current_key;
        bool is_nonblock;
    } nonblock_info;
};

struct list2 *window_queue;
int window_queue_mutex = -1;
struct framebuffer *buffer;

void win_draw_rect(struct window *win, struct rect area, struct framebuffer *fb)
{

    size_t x1 = MAX(WIN_X(win), area.x);
    size_t y1 = MAX(WIN_Y(win), area.y);

    size_t x2 = MIN(WIN_X(win) + WIN_W(win), area.x + area.w);
    size_t y2 = MIN(WIN_Y(win) + WIN_H(win), area.y + area.h);

    size_t draw_height = y2 - y1;
    size_t draw_width = x2 - x1;

    if (x1 >= x2 || y1 >= y2)
    {
        return;
    }

    for (size_t y = 0; y < draw_height; y++)
    {
        size_t fb_y = y1 + y;
        size_t win_y = (fb_y - WIN_Y(win));

        size_t fb_offset = (fb_y * fb->pitch) + (x1 * fb->pixel_width);

        size_t win_offset = (win_y * WIN_W(win) + (x1 - WIN_X(win))) * 4;

        memcpy(fb->buffer + fb_offset, win->fb.buffer + win_offset,
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
    user_mutex_lock(window_queue_mutex);

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

        /* TODO: struct framebuffer (or struct window) should have a member of
         * type struct rect */
        struct rect win_rect = WIN_RECT(win);
        if (rect_intersect(win_rect, area))
        {
            /* if (win != ent->win && win != mouse) */
            /* { */
            /*     /1* printf("intersect\n"); *1/ */
            /* } */
            win_draw_rect(ent2->win, area, fb);
        }
    }
    user_mutex_unlock(window_queue_mutex);
}

#define WINDOW_DECORATION_ROWS 10
struct window_queue_entry *focused = NULL;

/* correctly applys window decorations to newly focused window */
void change_focus(struct window_queue_entry *new_focus)
{

    for (size_t y = 0; y < WINDOW_DECORATION_ROWS; y++)
    {
        for (size_t x = 0; x < WIN_W(new_focus->win); x++)
        {
            *(u32 *)(new_focus->win->fb.buffer + y * WIN_W(new_focus->win) * 4 +
                     (x * 4)) = 0xFF00FF;
        }
    }
    struct rect new_rect = WIN_RECT(new_focus->win);
    update_area_up(
        new_focus,
        RECT_INIT(new_rect.x, new_rect.y, new_rect.w, WINDOW_DECORATION_ROWS),
        buffer);

    for (size_t y = 0; y < WINDOW_DECORATION_ROWS; y++)
    {
        for (size_t x = 0; x < WIN_W(focused->win); x++)
        {
            *(u32 *)(focused->win->fb.buffer + y * WIN_W(focused->win) * 4 +
                     (x * 4)) = 0;
        }
    }

    struct rect prev_rect = WIN_RECT(focused->win);
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
    out->x = x;
    out->y = y;
    out->fb = FRAMEBUFFER_INIT(w, h, w * 4, 32, 4, malloc(w * h * 4));

    return out;
}

struct window *new_client_window(size_t x, size_t y, size_t w, size_t h)
{
    struct window *out = malloc(sizeof(struct window));
    out->x = x;
    out->y = y;

    /* allocate extra space for decorations */
    size_t height = h + WINDOW_DECORATION_ROWS;
    out->fb = FRAMEBUFFER_INIT(w, height, w * 4, 32, 4, malloc(w * height * 4));

    return out;
}

void draw_window(struct framebuffer *b, struct window *win)
{
    for (size_t y = 0; y < WIN_H(win) - 1; y++)
    {
        memcpy(b->buffer + (((y + WIN_Y(win)) * b->pitch) +
                            (WIN_X(win) * b->pixel_width)),
               win->fb.buffer + (y * WIN_W(win) * 4), WIN_W(win) * 4);
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

static void disconnect_flip_image_w(struct window_queue_entry *ent,
                                    size_t num_pixels, size_t sleep_ticks,
                                    int pixel_factor)
{

    struct window *w = ent->win;
    for (size_t y = 0; y < WIN_H(w); y++)
    {
        if (y % num_pixels == 0)
        {
            struct rect area = WIN_RECT(w);
            area.h = y;

            update_area_up(ent, area, buffer);
            sleep(sleep_ticks);
        }

        for (size_t x = 0; x < WIN_W(w) / 2; x++)
        {
            size_t reverse_x = WIN_W(w) - x;
            u32 reverse_color =
                framebuffer_get_pixel(&w->fb, reverse_x, y) * pixel_factor;
            u32 color = framebuffer_get_pixel(&w->fb, x, y) * pixel_factor;
            framebuffer_put_pixel(&w->fb, color, reverse_x, y);
            framebuffer_put_pixel(&w->fb, reverse_color, x, y);
        }
    }
}
static void disconnect_flip_image_h(struct window_queue_entry *ent,
                                    size_t num_pixels, size_t sleep_ticks,
                                    int pixel_factor)
{

    struct window *w = ent->win;
    for (size_t y = 0; y < WIN_H(w); y++)
    {
        if (y % num_pixels == 0)
        {
            struct rect area = WIN_RECT(w);
            area.h = y;

            update_area_up(ent, area, buffer);
            sleep(sleep_ticks);
        }

        if (y > WIN_H(w) / 2)
        {
            continue;
        }

        for (size_t x = 0; x < WIN_W(w); x++)
        {
            size_t reverse_y = WIN_H(w) - y;
            u32 reverse_color =
                framebuffer_get_pixel(&w->fb, x, reverse_y) * pixel_factor;
            u32 color = framebuffer_get_pixel(&w->fb, x, y) * pixel_factor;
            framebuffer_put_pixel(&w->fb, color, x, reverse_y);
            framebuffer_put_pixel(&w->fb, reverse_color, x, y);
        }

        /* memset(w->fb.buffer + (y * WIN_W(w) * 4), 0, WIN_W(w) * 4); */
    }
}

int current_client = 1;

size_t y = 10;
size_t x = 400;

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

    struct window *w = new_client_window(x, y, win_info.width, win_info.height);
    y += win_info.height - 30;
    x += 40;

    struct window_queue_entry *ent = malloc(sizeof(struct window_queue_entry));
    ent->list = LIST2_INIT();
    ent->win = w;
    ent->conn = conn;
    printf("%d\n", win_info.is_nonblock);
    ent->nonblock_info.is_nonblock = win_info.is_nonblock;
    ent->nonblock_info.current_key = 0;

    user_mutex_lock(window_queue_mutex);
    if (window_queue == NULL)
    {
        window_queue = &ent->list;
    }
    else
    {
        list2_add_front(&ent->list, &window_queue);
    }
    user_mutex_unlock(window_queue_mutex);

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

            for (size_t y = 0; y < (WIN_H(w) - WINDOW_DECORATION_ROWS) - 1; y++)
            {
                size_t server_offset =
                    (y + WINDOW_DECORATION_ROWS) * WIN_W(w) * 4;
                size_t client_offset = y * WIN_W(w) * 4;

                memcpy(w->fb.buffer + server_offset, client_buf + client_offset,
                       WIN_W(w) * 4);
            }

            update_area_up(ent, WIN_RECT(w), buffer);
            /* draw_windows(buffer); */

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
            /* printf("sending %c\n", input); */
            retval = send(socket, &input, sizeof(char));
            if (retval == -ENOTSOCK)
            {
                goto disconnect;
            }
            /* printf("sent it\n"); */
            break;
        }
        case WM_RES_KEY_NONBLOCK:
        {
            char c = 0;
            if (ent->nonblock_info.current_key != 0)
            {
                c = ent->nonblock_info.current_key;
                ent->nonblock_info.current_key = 0;
            }
            send(socket, &c, 1);
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

    size_t num_pixels = WIN_H(w) - 1;
    size_t sleep_ticks = 3;

    disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 4);
    disconnect_flip_image_h(ent, num_pixels, sleep_ticks, -5);
    disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 1);
    disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 1);
    disconnect_flip_image_h(ent, num_pixels, sleep_ticks, 8);
    disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 1);

    disconnect_flip_image_h(ent, num_pixels, sleep_ticks, 1);
    disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 4);
    disconnect_flip_image_h(ent, num_pixels, sleep_ticks, 1);
    disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 1);
    disconnect_flip_image_h(ent, num_pixels, sleep_ticks, 2);
    disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 1);
    disconnect_flip_image_h(ent, num_pixels, sleep_ticks, 4);
    disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 1);

    user_mutex_lock(window_queue_mutex);
    list2_remove(&ent->list, &window_queue);
    user_mutex_unlock(window_queue_mutex);

    while (1)
    {
        sched_yield();
    }
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
                if (focused->nonblock_info.is_nonblock)
                {
                    focused->nonblock_info.current_key = c;
                }
                else
                {
                    struct wm_connection *conn = focused->conn;
                    current_keyboard_char = c;

                    send(conn->input_socket, &c, sizeof(char));
                }
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

        struct rect old_mouse = WIN_RECT(mouse);

        WIN_X(mouse) += ev->x_change;
        WIN_Y(mouse) += ev->y_change;

        if ((int)WIN_X(mouse) < 0)
        {
            WIN_X(mouse) = 0;
        }

        if ((int)WIN_Y(mouse) < 0)
        {
            WIN_Y(mouse) = 0;
        }

        draw_window(buffer, mouse);
        /* TODO: dont pass null */
        update_area_up(NULL, WIN_RECT(mouse), buffer);
        update_area_up(NULL, old_mouse, buffer);

        if (ev->buttons.left_click && drag_count < 3)
        {

            drag_count++;

            /* TODO: this should be its own function that traverses the list
             * backwards */
            user_mutex_lock(window_queue_mutex);
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

                if (WIN_X(mouse) > WIN_X(win) &&
                    WIN_X(mouse) < WIN_X(win) + WIN_W(win) &&
                    WIN_Y(mouse) > WIN_Y(win) &&
                    WIN_Y(mouse) < WIN_Y(win) + WIN_H(win))
                {
                    new_focus = queue_ent;
                    break;
                }
            }
            user_mutex_unlock(window_queue_mutex);

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
                struct window *win = focused->win;
                struct rect old_area = WIN_RECT(win);

                WIN_X(win) += ev->x_change;
                WIN_Y(win) += ev->y_change;

                struct rect move_area = {
                    .x = WIN_X(win),
                    .y = WIN_Y(win),
                    .w = abs(ev->x_change + WIN_W(win)),
                    .h = abs(ev->y_change + WIN_H(win)),
                };
                /* printf("%d ", move_area.h); */

                /* update_area_up(focused, move_area, buffer); */
                update_area_up(focused, WIN_RECT(win), buffer);
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

int accept(int socket)
{
    return syscall_wrapper_asm(SYS_accept, socket, 0, 0, 0, 0);
}
int main()
{
    init_libc();
    printf("test\n");

    /* printf("window manager\n"); */
    /* clone((ptr)thread1); */

    int server_sock = syscall_wrapper_asm(SYS_create_socket_fs,
                                          (ptr) "wm_socket", 0, 0, 0, 0);

    window_queue_mutex = user_mutex_create();

    size_t mouse_w = 10;
    size_t mouse_h = 10;
    mouse = new_window(0, 0, mouse_w, mouse_h);

    for (size_t y = 0; y < mouse_h; y++)
    {
        for (size_t x = 0; x < mouse_w; x++)
        {
            /* u32 val = x * x * x * y * y * y * y; */
            u32 val = 0xFF;
            framebuffer_put_pixel(&mouse->fb, val, x, y);
            /* *(u32 *)(mouse->fb.buffer + (x * 4) + (y * mouse_w * 4)) = val;
             */
        }
    }
    /* memset(mouse->fb.buffer, 255, 10 * 10 * 4); */

    buffer = framebuffer_alloc();
    draw_window(buffer, mouse);

    /* this might be bad because this is a stack allocation */
    struct window_queue_entry mouse_ent = {.win = mouse, .list = LIST2_INIT()};
    window_queue = &mouse_ent.list;
    mouse_ent.nonblock_info.is_nonblock = false;
    mouse_ent.nonblock_info.current_key = 0;

    clone((ptr)mouse_thread);
    clone((ptr)keyboard_thread);

    /* execv("/elf_test", (char *[]){"elf_test", NULL}); */
    /* execv("imgview", (char *[]){"imgview", "osdev.tga", NULL}); */
    execv("/elf_test", (char *[]){"elf_test", NULL});
    execv("/elf_test", (char *[]){"elf_test", NULL});
    /* execv("tron", (char *[]){"imgview", "osdev.tga", NULL}); */

    while (true)
    {
        int client_sock = accept(server_sock);
        printf("%d\n", client_sock);
        /* printf("socket num:%d\n", client_sock); */
        /* send(server_sock, &client_sock, sizeof(int)); */
        /* printf("sent\n"); */
        current_client = client_sock;

        clone((ptr)(process_connection));
    }
    /* process_connection(client_sock); */

    exit(0);
}
