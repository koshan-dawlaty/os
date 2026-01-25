#ifndef WM_LIB_H
#define WM_LIB_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/syscall.h>

#define __packed __attribute__((packed))

struct window
{
    size_t x;
    size_t y;
    size_t w;
    size_t h;

    void *buffer;
};

struct client_window
{
    size_t w;
    size_t h;

    void *buffer;
};

static inline void client_draw_pixel(struct client_window *w, size_t x,
                                     size_t y, uint32_t color)
{
    size_t location = y * w->h * 4 + x * 4;

    uint8_t *pixel = (uint8_t *)w->buffer + location;
    pixel[0] = color & 255;         // BLUE
    pixel[1] = (color >> 8) & 255;  // GREEN
    pixel[2] = (color >> 16) & 255; // RED
}

/* this is copied from the kernel definition, it might change and break
 * everything and that would be bad */
struct framebuffer
{
    size_t width;
    size_t height;
    size_t pitch;
    size_t depth;
    size_t pixel_width; /* bytes */
    void *buffer;
} __packed;

struct wm_inital
{
    size_t height;
    size_t width;
} __packed;

struct wm_second
{
    int mem_id;
} __packed;

#define WM_RES_RENDER 1

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

void client_draw_char(struct client_window *w, struct PSF_font *font, char c,
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

#endif
