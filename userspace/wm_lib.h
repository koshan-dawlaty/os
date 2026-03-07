#ifndef WM_LIB_H
#define WM_LIB_H

#include <lib/framebuffer.h>
#include <lib/rect.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>

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
    struct framebuffer fb;
};

#define WM_CLIENT_WIDTH(client) (client.fb.width)
#define WM_CLIENT_HEIGHT(client) (client.fb.height)

static inline void client_draw_pixel(struct wm_client client, size_t x,
                                     size_t y, uint32_t color)
{

    framebuffer_put_pixel(&client.fb, color, x, y);
    // w->buffer
    // size_t location = y * w->w * 4 + x * 4;

    // uint8_t *pixel = (uint8_t *)w->buffer + location;
    // pixel[0] = color & 255;         // BLUE
    // pixel[1] = (color >> 8) & 255;  // GREEN
    // pixel[2] = (color >> 16) & 255; // RED
}

struct wm_inital
{
    size_t height;
    size_t width;
} __packed;

struct wm_second
{
    int mem_id;
} __packed;

/* TODO: some way of making these increment automatically? */
#define WM_RES_RENDER 1
#define WM_RES_KEY 2

typedef int wm_res_code;

struct wm_client_message
{
    wm_res_code code;
} __packed;

void framebuffer_pixel_write(struct framebuffer *f, uint8_t *pixel, int color);
void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color);

struct PSF_header
{
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t num_glyph;
    uint32_t bytes_per_glyph;
    uint32_t height;
    uint32_t width;
} __packed;

struct PSF_font
{
    struct PSF_header header;
    uint8_t *content;
};

void client_draw_char(struct wm_client client, struct PSF_font *font, char c,
                      size_t x, size_t y);
struct PSF_font *font_parse(int fd);

typedef uintptr_t ptr;
extern int syscall_wrapper_asm(ptr ax, ptr bx, ptr cx, ptr dx, ptr si, ptr di);

static inline struct framebuffer *framebuffer_alloc()
{
    struct framebuffer *out = NULL;
    syscall_wrapper_asm(SYS_get_framebuffer, (ptr)&out, 0, 0, 0, 0);

    return out;
}

static inline int socket_create()
{
    return syscall_wrapper_asm(SYS_create_socket, 0, 0, 0, 0, 0);
}

static inline int send(int socket, void *buf, size_t len)
{
    return syscall_wrapper_asm(SYS_send, socket, (ptr)buf, len, 0, 0);
}

static inline int recv(int socket, void *buf, size_t len)
{
    return syscall_wrapper_asm(SYS_recv, socket, (ptr)buf, len, 0, 0);
}

static inline int clone(ptr ip)
{
    return syscall_wrapper_asm(SYS_clone, ip, 0, 0, 0, 0);
}

static inline int shared_mem_alloc(void **addr, size_t len)
{
    return syscall_wrapper_asm(SYS_shared_mem_alloc, (ptr)addr, len, 0, 0, 0);
}

static inline int shared_mem_get(int id, void **p)
{
    return syscall_wrapper_asm(SYS_shared_mem_get, id, (ptr)p, 0, 0, 0);
}

void wm_render(struct wm_client client);
char wm_get_key(struct wm_client client);
struct wm_client wm_client_connect(size_t height, size_t width);

#endif
