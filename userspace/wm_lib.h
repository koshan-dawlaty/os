#ifndef WM_LIB_H
#define WM_LIB_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

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

void client_draw_char(struct client_window *w, struct PSF_font *font, char c,
                      size_t x, size_t y);
struct PSF_font *font_parse(int fd);

#endif
