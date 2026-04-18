#include "wm_lib.h"

#include <container_of.h>
#include <errno.h>
#include <extension.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/framebuffer.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/rect.h>
#include <lib/utils.h>
#include <sched.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>
#include <userspace/mouse.h>

struct wm_connection
{
    int client_socket;
    int client_key_socket;
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

struct window *mouse;
struct window_queue_entry *mouse_queue_ent;
struct window_queue_entry *background_queue_ent;
struct window *background;

struct list2 *window_queue;
int window_queue_mutex = -1;
struct framebuffer *buffer;

void win_draw_rect(struct window *win, struct rect area, struct framebuffer *fb)
{

    size_t x1 = MAX(WIN_X(win), (size_t)area.x);
    size_t y1 = MAX(WIN_Y(win), (size_t)area.y);

    size_t x2 = MIN(WIN_X(win) + WIN_W(win), (size_t)(area.x + area.w));
    size_t y2 = MIN(WIN_Y(win) + WIN_H(win), (size_t)(area.y + area.h));

    size_t draw_height = y2 - y1;
    size_t draw_width = x2 - x1;

    if (draw_height + win->y > fb->height)
    {
        if (fb->height < win->y)
        {
            draw_height = 0;
        }
        else
        {
            draw_height = fb->height - win->y;
        }
    }

    if (draw_width + win->x >= fb->width)
    {
        if (fb->width < win->x)
        {
            draw_width = 0;
        }
        else
        {
            draw_width = fb->width - win->x;
        }
    }

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

u32 color_num(size_t num)
{
    switch (num)
    {
    case 0:
        return 0x0000FF;
    case 1:
        return 0xFF00FF;
    case 2:
        return 0xFFFF00;
    case 3:
        return 0x00FF00;
    case 4:
        return 0x00FFFF;
    }
    return 0xFFFFFF;
}

struct array compute_visible(struct window_queue_entry *win_ent,
                             struct rect area)
{

    struct window *win = win_ent->win;

    struct array visible = ARRAY_INIT();
    struct rect intersection = rect_get_intersect(WIN_RECT(win), area);
    if (rect_eq(intersection, RECT_INIT(0, 0, 0, 0)))
    {
        return visible;
    }
    array_add(&visible, &intersection, sizeof(struct rect));
    /* array_add(&visible, &WIN_RECT(win), sizeof(struct rect)); */

    struct list2 *pos;
    for (pos = win_ent->list.next; pos != NULL; pos = pos->next)
    /* list2_for_each(pos, &win_ent->list) */
    /* list2_for_each(pos, window_queue) */
    {
        struct window_queue_entry *ent =
            containerof(pos, struct window_queue_entry, list);
        struct rect current = rect_get_intersect(WIN_RECT(ent->win), area);
        if (rect_eq(current, RECT_INIT(0, 0, 0, 0)))
            continue;

        struct array new_regions = ARRAY_INIT();

        for (size_t i = 0; i < ARRAY_LEN(visible, sizeof(struct rect)); i++)
        {
            struct rect *r = array_get(&visible, i, sizeof(struct rect));
            if (!rect_intersect(current, *r))
            {
                array_add(&new_regions, r, sizeof(struct rect));
            }
            else
            {
                /* struct rect *results = malloc(sizeof(struct rect) * 4);
                 */
                struct rect results[4] = {0};
                u8 num = rect_split_intersect(*r, current, results);
                /* logd("num:%d", num); */
                for (size_t j = 0; j < num; j++)
                {
                    struct rect result = results[j];
                    /* logd("this"); */
                    /* logd("adding:%d, %d w:%d, h:%d", result.x, result.y, */
                    /*      result.w, result.h); */
                    array_add(&new_regions, &results[j], sizeof(struct rect));
                    /* framebuffer_draw_rect(buffer, result.x, result.y,
                     * result.w, */
                    /*                       result.h, color_num(j)); */
                }
            }
        }
        /* logd("loop"); */
        array_free(&visible);
        visible = new_regions;
    }
    return visible;
}

void update_area_up(struct rect area, struct framebuffer *fb)
{
    user_mutex_lock(window_queue_mutex);

    struct list2 *pos;
    list2_for_each(pos, window_queue)
    {
        struct window_queue_entry *ent =
            containerof(pos, struct window_queue_entry, list);

        struct array visible = compute_visible(ent, area);

        /* logd("%d", ARRAY_LEN(visible, sizeof(struct rect))); */

        for (size_t i = 0; i < ARRAY_LEN(visible, sizeof(struct rect)); i++)
        {
            struct rect *r = array_get(&visible, i, sizeof(struct rect));
            /* logd("%p", r); */
            // struct rect intersect = rect_get_intersect(*r,
            // WIN_RECT(ent->win));
            /* struct rect intersect = rect_get_intersect(*r, area); */
            struct rect intersect = *r;
            /* logd("intersect:x:%d y:%d w:%d h:%d", intersect.x, intersect.y,
             */
            /*      intersect.w, intersect.h); */

            if (!rect_eq(intersect, RECT_INIT(0, 0, 0, 0)))
            {
                win_draw_rect(ent->win, intersect, fb);
            }
            /* logd("drawing:%d, %d w:%d, h:%d", r->x, r->y, r->w,
             * r->h); */
            /* struct list2 *pos; */
            /* list2_for_each(pos, window_queue) */
            /* { */
            /*     struct window_queue_entry *ent = */
            /*         containerof(pos, struct window_queue_entry,
             * list); */
            /*     win_draw_rect(ent->win, *r, buffer); */
            /* } */
        }
        array_free(&visible);
    }

    user_mutex_unlock(window_queue_mutex);
}

void draw_window(struct framebuffer *b, struct window *win)
{
    update_area_up(WIN_RECT(win), b);
}

void move_window(struct window_queue_entry *ent, int x, int y,
                 struct framebuffer *fb)
{

    struct window *win = ent->win;
    struct rect old_area = WIN_RECT(ent->win);

    WIN_X(win) += x;
    WIN_Y(win) += y;

    if ((int)WIN_X(win) <= 0)
    {
        WIN_X(win) = 0;
    }
    if ((int)WIN_Y(win) <= 0)
    {
        WIN_Y(win) = 0;
    }

    if (WIN_X(win) + WIN_W(win) >= fb->width)
    {
        WIN_X(win) = fb->width - WIN_W(win);
    }
    if (WIN_Y(win) + WIN_H(win) >= fb->height)
    {
        WIN_Y(win) = fb->height - WIN_H(win);
    }

    struct rect new_area = WIN_RECT(ent->win);

    struct rect result[4] = {0};
    u8 num_rects = rect_split_intersect(old_area, new_area, result);

    /* moved far enough away to not intersect */
    if (num_rects == 0)
    {
        update_area_up(old_area, buffer);
    }

    for (size_t i = 0; i < num_rects; i++)
    {
        struct rect r = result[i];

        /* logd("damage:x:%d y:%d w:%d h:%d", r.x, r.y, r.w, r.h);
         */
        update_area_up(r, buffer);
        /* framebuffer_draw_rect(buffer, r.x, r.y, r.w, r.h, */
        /*                       color_num(i)); */
    }
    update_area_up(WIN_RECT(win),
                   buffer); /* check if this is useful, i think it is */
}

void move_window_step(struct window_queue_entry *ent, int x, int y,
                      int num_steps, size_t s)
{
    for (size_t i = 0; i < num_steps; i++)
    {
        move_window(ent, x / num_steps, y / num_steps, buffer);
        sleep(s);
    }
}

#define WINDOW_DECORATION_ROWS 10
struct window_queue_entry *focused = NULL;

/* correctly applys window decorations to newly focused window */
void change_focus(struct window_queue_entry *new_focus)
{

    /* TODO: the focusable windows should maybe be stored in a separate way so
     * this does not need to happen */ /* this check does not techincally need to happen right now, any callers of this will not foucs these windows */
    if (new_focus == background_queue_ent || new_focus == mouse_queue_ent)
    {
        return;
    }

    for (size_t y = 0; y < WINDOW_DECORATION_ROWS; y++)
    {
        for (size_t x = 0; x < WIN_W(new_focus->win); x++)
        {
            framebuffer_put_pixel(&new_focus->win->fb, 0xFF00FF, x, y);

            /* *(u32 *)(new_focus->win->fb.buffer + y *
             * WIN_W(new_focus->win) * 4 + */
            /*          (x * 4)) = 0xFF00FF; */
        }
    }
    struct rect new_rect = WIN_RECT(new_focus->win);
    update_area_up(
        RECT_INIT(new_rect.x, new_rect.y, new_rect.w, WINDOW_DECORATION_ROWS),
        buffer);

    for (size_t y = 0; y < WINDOW_DECORATION_ROWS; y++)
    {
        for (size_t x = 0; x < WIN_W(focused->win); x++)
        {
            /* *(u32 *)(focused->win->fb.buffer + y * WIN_W(focused->win) *
             * 4 +
             */
            /*          (x * 4)) = 0; */
            framebuffer_put_pixel(&focused->win->fb, 0xFF, x, y);
        }
    }

    struct rect prev_rect = WIN_RECT(focused->win);
    update_area_up(RECT_INIT(prev_rect.x, prev_rect.y, prev_rect.w,
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
    /* the shared memory buffer could be used here, but the way it works
     * now, it will break window decorations */
    out->fb = FRAMEBUFFER_INIT(w, height, w * 4, 32, 4, malloc(w * height * 4));
    logd("%p", out->fb.buffer + (w * height * 4));

    return out;
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

            update_area_up(area, buffer);
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

            update_area_up(area, buffer);
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

int accept(int socket)
{
    return syscall_wrapper_asm(SYS_accept, socket, 0, 0, 0, 0);
}

int current_client = 1;
int current_key_socket;

size_t y = 10;
size_t x = 400;

void process_connection()
{
    int socket;
    socket = current_client;
    int key_socket = current_key_socket;

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
    conn->client_key_socket = key_socket;

    struct window *w = new_client_window(x, y, win_info.width, win_info.height);
    /* y += win_info.height - 30; */
    /* x += 40; */

    struct window_queue_entry *ent = malloc(sizeof(struct window_queue_entry));
    ent->list = LIST2_INIT();
    ent->win = w;
    ent->conn = conn;
    ent->nonblock_info.is_nonblock = win_info.is_nonblock;
    ent->nonblock_info.current_key = 0;

    user_mutex_lock(window_queue_mutex);
    if (window_queue == NULL)
    {
        window_queue = &ent->list;
    }
    else
    {
        ent->list.prev = window_queue;
        ent->list.next = window_queue->next;
        window_queue->next->prev = &ent->list;

        window_queue->next = &ent->list;
        /* list2_add_front(&ent->list, &window_queue); */
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

            update_area_up(WIN_RECT(w), buffer);
            /* draw_window(buffer, ent->win); */

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

            if (input == '\\')
            {
                /* TODO: this might not properly close the socket */
                close(conn->client_socket);
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
    size_t sleep_ticks = 20;
    size_t dist = 100;
    size_t num_ticks = 10;

    for (size_t i = 0; i < 3; i++)
    {
        move_window_step(ent, dist, dist, num_ticks, sleep_ticks);
        sleep(sleep_ticks);
        move_window_step(ent, -dist, dist, num_ticks, sleep_ticks);
        sleep(sleep_ticks);
        move_window_step(ent, -dist, -dist, num_ticks, sleep_ticks);
        sleep(sleep_ticks);
        move_window_step(ent, dist, -dist, num_ticks, sleep_ticks);

        dist += 50;
    }

    /* for (size_t i = 0; i < 10; i++) */
    /* { */
    /*     disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 1); */
    /*     disconnect_flip_image_h(ent, num_pixels, sleep_ticks, 1); */
    /*     disconnect_flip_image_w(ent, num_pixels, sleep_ticks, 1); */
    /*     disconnect_flip_image_h(ent, num_pixels, sleep_ticks, 1); */
    /* } */

    struct rect old_area = WIN_RECT(ent->win);

    user_mutex_lock(window_queue_mutex);
    list2_remove(&ent->list, &window_queue);
    user_mutex_unlock(window_queue_mutex);

    update_area_up(old_area, buffer);

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
            if (ent == background_queue_ent)
            {
                ent = containerof(window_queue->next, struct window_queue_entry,
                                  list);
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

                    /* printf("sending key to:%d\n", conn->input_socket); */
                    send(conn->client_key_socket, &c, sizeof(char));
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

        move_window(mouse_queue_ent, ev->x_change, ev->y_change, buffer);
        /* update_area_down(mouse_queue_ent, old_mouse, buffer); */

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

            /* logd("%p", prev->prev->prev); */

            struct window_queue_entry *new_focus = NULL;

            for (pos = prev; pos != NULL; pos = pos->prev)
            {

                struct window_queue_entry *queue_ent =
                    LIST_ENTRY(pos, struct window_queue_entry, list);

                if (queue_ent == mouse_queue_ent ||
                    queue_ent == background_queue_ent)
                {
                    continue;
                }

                struct window *win = queue_ent->win;

                if (WIN_X(mouse) > WIN_X(win) &&
                    WIN_X(mouse) < WIN_X(win) + WIN_W(win) &&
                    WIN_Y(mouse) > WIN_Y(win) &&
                    WIN_Y(mouse) < WIN_Y(win) + WIN_H(win))

                {
                    /* logd("intersect"); */
                    new_focus = queue_ent;
                    break;
                }
            }
            user_mutex_unlock(window_queue_mutex);

            if (new_focus != NULL)
            {
                /* memset(focused->win->buffer, 127,
                 * focused->win->w * 4 * 10);
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
                move_window(focused, ev->x_change, ev->y_change, buffer);

                /* printf("%d ", move_area.h); */

                /* struct rect *rects = malloc(sizeof(struct rect) * 4); */
                /* u8 num_rects = rect_split_intersect( */
                /*     WIN_RECT(background), WIN_RECT(focused->win), rects); */
                /* for (size_t i = 0; i < num_rects; i++) */
                /* { */
                /*     win_draw_rect(background, rects[i], buffer); */
                /*     /1* struct rect r = rects[i]; *1/ */
                /*     /1* u32 color; *1/ */
                /*     /1* switch (i) *1/ */
                /*     /1* { *1/ */
                /*     /1* case 0: *1/ */
                /*     /1*     color = 0xFFFF; *1/ */
                /*     /1*     break; *1/ */
                /*     /1* case 1: *1/ */
                /*     /1*     color = 0xFFFF00; *1/ */
                /*     /1*     break; *1/ */
                /*     /1* case 2: *1/ */
                /*     /1*     color = 0xFFFFFF; *1/ */
                /*     /1*     break; *1/ */
                /*     /1* case 3: *1/ */
                /*     /1*     color = 0x00FF00; *1/ */
                /*     /1*     break; *1/ */
                /*     /1* default: *1/ */
                /*     /1*     color = 0xFF00FF; *1/ */
                /*     /1* } *1/ */
                /*     /1* framebuffer_draw_rect(buffer, r.x, r.y, r.w, r.h, */
                /*      * color); */
                /*      *1/ */
                /* } */
                /* draw_window(buffer, focused->win); */

                /* struct rect current_area = WIN_RECT(win); */
                /* if (rect_intersect(old_area, current_area)) */
                /* { */
                /*     struct rect intersect = */
                /*         rect_get_intersect(current_area, old_area); */
                /*     struct rect *results = malloc(sizeof(struct rect) * 4);
                 */
                /*     u8 num_rects = */
                /*         rect_split_intersect(old_area, current_area,
                 * results); */
                /*     for (size_t i = 0; i < num_rects; i++) */
                /*     { */
                /*         struct rect r = results[i]; */
                /*         // framebuffer_draw_rect(buffer, r.x, r.y, r.w, r.h,
                 */
                /*         //                      0xFF0000); */
                /*         update_area_up(focused, r, buffer); */
                /*     } */
                /*     update_area_up(focused, intersect, buffer); */
                /*     //  framebuffer_draw_rect(buffer, intersect.x,
                 * intersect.y, */
                /*     //                       intersect.w, intersect.h, 0xFF);
                 */
                /* } */

                /* update_area_up(focused, old_area, buffer); */
                /* update_area_up(focused, move_area, buffer); */
                /* update_area_up(focused, */
                /*                (struct rect){.x = 0, */
                /*                              .y = 0, */
                /*                              .h =
                 * buffer->height, */
                /*                              .w =
                 * buffer->width}, */
                /*                buffer); */
            }
        }
        else
        {
            drag_count = 0;
        }

        /* printf("%d, %d\n", mouse->x, mouse->y); */

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

    int server_sock = syscall_wrapper_asm(SYS_create_socket_fs,
                                          (ptr) "wm_socket", 0, 0, 0, 0);

    int input_sock = syscall_wrapper_asm(SYS_create_socket_fs,
                                         (ptr) "input_socket", 0, 0, 0, 0);

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
            /* *(u32 *)(mouse->fb.buffer + (x * 4) + (y * mouse_w * 4)) =
             * val;
             */
        }
    }
    /* memset(mouse->fb.buffer, 255, 10 * 10 * 4); */

    buffer = framebuffer_alloc();

    background = new_window(0, 0, buffer->width, buffer->height);
    framebuffer_draw_rect(&background->fb, 0, 0, WIN_W(background),
                          WIN_H(background), 0x233826);

    /* this might be bad because these are stack allocations */
    mouse_queue_ent = malloc(sizeof(struct window_queue_entry));
    mouse_queue_ent->conn = NULL;
    mouse_queue_ent->win = mouse;
    mouse_queue_ent->list = LIST2_INIT();
    window_queue = &mouse_queue_ent->list;
    mouse_queue_ent->nonblock_info.is_nonblock = false;
    mouse_queue_ent->nonblock_info.current_key = 0;

    background_queue_ent = malloc(sizeof(struct window_queue_entry));
    background_queue_ent->win = background;
    background_queue_ent->list = LIST2_INIT();
    background_queue_ent->conn = NULL;
    background_queue_ent->nonblock_info.is_nonblock = false;
    background_queue_ent->nonblock_info.current_key = 0;

    list2_add_front(&background_queue_ent->list, &window_queue);

    draw_window(buffer, mouse);
    draw_window(buffer, background);

    clone((ptr)mouse_thread);
    clone((ptr)keyboard_thread);

    /* execv("/elf_test", (char *[]){"elf_test", NULL}); */
    /* execv("imgview", (char *[]){"imgview", "osdev.tga", NULL}); */
    /* execv("imgview", (char *[]){"imgview", "osdev.tga", NULL}); */
    /* execv("imgview", (char *[]){"imgview", "osdev.tga", NULL}); */
    execv("/elf_test", (char *[]){"elf_test", NULL});
    execv("imgview", (char *[]){"imgview", "dog.tga", NULL});
    execv("imgview", (char *[]){"imgview", "dog.tga", NULL});
    execv("imgview", (char *[]){"imgview", "dog.tga", NULL});
    /* execv("imgview", (char *[]){"imgview", "dog.tga", NULL}); */
    /* execv("/elf_test", (char *[]){"elf_test", NULL}); */
    /* execv("/elf_test", (char *[]){"elf_test", NULL}); */
    /* execv("tron", (char *[]){"imgview", "osdev.tga", NULL}); */

    while (true)
    {
        int client_sock = accept(server_sock);
        int key_socket = accept(input_sock);
        /* printf("socket num:%d\n", client_sock); */
        /* send(server_sock, &client_sock, sizeof(int)); */
        /* printf("sent\n"); */
        current_client = client_sock;
        current_key_socket = key_socket;

        clone((ptr)(process_connection));
    }
    /* process_connection(client_sock); */

    exit(0);
}
