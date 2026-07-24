#ifndef WM_LIB_H
#define WM_LIB_H

#include "lib/log.h"
#include "userspace/mouse.h"
#include <fcntl.h>
#include <lib/font.h>
#include <lib/framebuffer.h>
#include <lib/rect.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>

#define __packed __attribute__((packed))

struct window
{
    size_t x;
    size_t y;
    struct framebuffer fb;
};

#define WIN_X(win) (win->x)
#define WIN_Y(win) (win->y)
#define WIN_W(win) (win->fb.width)
#define WIN_H(win) (win->fb.height)

#define WIN_RECT(win)                                                          \
    (RECT_INIT(WIN_X(win), WIN_Y(win), WIN_W(win), WIN_H(win)))

struct client_window
{
    size_t w;
    size_t h;

    void *buffer;
};

struct wm_client
{
    int socket;
    int key_socket;
    int mem_id;
    struct framebuffer fb;
};

#define WM_MESSAGE_INVALID -1
#define WM_MESSAGE_KEY 1
#define WM_MESSAGE_MOUSE 2
#define WM_MESSAGE_RESIZE 3
typedef u8 wm_message_type_t;

struct wm_message_header
{
    wm_message_type_t type;
    size_t len;
} __packed;

#define WM_MESSAGE_KEY_HEADER                                                  \
    ((struct wm_message_header){.type = WM_MESSAGE_KEY, .len = 1})

#define WM_MESSAGE_MOUSE_HEADER                                                \
    ((struct wm_message_header){.type = WM_MESSAGE_MOUSE,                      \
                                .len = sizeof(struct mouse_event)})

struct wm_message_resize
{
    size_t new_w, new_h;
};
#define WM_MESSAGE_RESIZE_HEADER                                               \
    ((struct wm_message_header){.type = WM_MESSAGE_RESIZE,                     \
                                .len = sizeof(struct wm_message_resize)})

struct wm_message
{
    struct wm_message_header header;
    void *message_data;
} __packed;

#define WM_CLIENT_WIDTH(client) (client.fb.width)
#define WM_CLIENT_HEIGHT(client) (client.fb.height)

static inline void client_draw_pixel(struct wm_client client, size_t x,
                                     size_t y, uint32_t color)
{
    framebuffer_put_pixel(&client.fb, color, x, y);
}

static inline void client_draw_rect(struct wm_client client, size_t x, size_t y,
                                    size_t width, size_t height, u32 color)
{
    framebuffer_draw_rect(&client.fb, x, y, width, height, color);
}

struct wm_inital
{
    size_t height;
    size_t width;
    /* TODO: stupid, use good nonblocking socket implementation */
    bool is_nonblock;
    bool auto_foucs;
} __packed;

struct wm_second
{
    int mem_id;
} __packed;

/* TODO: some way of making these increment automatically? */
#define WM_RES_RENDER 1
#define WM_RES_KEY 2
#define WM_RES_KEY_NONBLOCK 4
#define WM_RES_RENDER_PARTIAL 5

typedef int wm_res_code;

struct wm_client_message
{
    wm_res_code code;
} __packed;

void framebuffer_pixel_write(struct framebuffer *f, uint8_t *pixel, int color);
void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color);

void client_draw_char(struct wm_client client, struct PSF_font *font, char c,
                      size_t x, size_t y);

void client_draw_string(struct wm_client client, struct PSF_font *font,
                        const char *str, size_t x, size_t y);

typedef uintptr_t ptr_t;
extern int syscall_wrapper_asm(ptr_t ax, ptr_t bx, ptr_t cx, ptr_t dx, ptr_t si,
                               ptr_t di);

static inline struct framebuffer *framebuffer_alloc()
{
    struct framebuffer *out = NULL;
    syscall_wrapper_asm(SYS_get_framebuffer, (ptr_t)&out, 0, 0, 0, 0);

    return out;
}

static inline int socket_create()
{
    return syscall_wrapper_asm(SYS_create_socket, 0, 0, 0, 0, 0);
}

static inline int send(int socket, void *buf, size_t len)
{
    return write(socket, buf, len);
}

static inline int recv(int socket, void *buf, size_t len)
{
    return read(socket, buf, len);
}

static inline int clone(ptr_t ip)
{
    return syscall_wrapper_asm(SYS_clone, ip, 0, 0, 0, 0);
}

static inline int shared_mem_alloc(void **addr, size_t len)
{
    return syscall_wrapper_asm(SYS_shared_mem_alloc, (ptr_t)addr, len, 0, 0, 0);
}

static inline int shared_mem_get(int id, void **p)
{
    return syscall_wrapper_asm(SYS_shared_mem_get, id, (ptr_t)p, 0, 0, 0);
}

static inline int shared_mem_resize(int id, size_t len, void **addr)
{
    return syscall_wrapper_asm(SYS_shared_mem_resize, id, len, (ptr_t)addr, 0,
                               0);
}

static inline int connect(int socket)
{
    return syscall_wrapper_asm(SYS_connect, socket, 0, 0, 0, 0);
}

void wm_render(struct wm_client client);
void wm_render_partial(struct wm_client client, struct rect rect);
char wm_get_key(struct wm_client client);
char wm_get_key_nonblock(struct wm_client client);
struct wm_client wm_client_connect(size_t height, size_t width);
/* makes this window in focus after connecting */
struct wm_client wm_client_connect_focus(size_t height, size_t width);
struct wm_client wm_client_connect_nonblock(size_t height, size_t width);

void wm_get_message(struct wm_client *client, struct wm_message *message);

#endif
