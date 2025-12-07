#include "memory/get_free_pages.h"
#include "psu_font.h"
#include <compiler.h>
#include <kernel/graphics.h>
#include <kernel/types.h>
#include <stdio.h>

struct PSF_font_header
{
    u32 magic;         /* magic bytes to identify PSF */
    u32 version;       /* zero */
    u32 headersize;    /* offset of bitmaps in file, 32 */
    u32 flags;         /* 0 if there's no unicode table */
    u32 numglyph;      /* number of glyphs */
    u32 bytesperglyph; /* size of each glyph */
    u32 height;        /* height in pixels */
    u32 width;         /* width in pixels */
} __packed;

struct PSF_font
{
    struct PSF_font_header header;
    u8 *fontdata;
};

#define PSF_FONT_MAGIC 0x864ab572
struct PSF_font parse_font(u8 *font_buf)
{
    struct PSF_font_header *header = (struct PSF_font_header *)(font_buf);
    if (header->magic != PSF_FONT_MAGIC)
    {
        printf("font might not work\n");
    }

    struct PSF_font out = {.header = *header,
                           .fontdata =
                               font_buf + sizeof(struct PSF_font_header)};

    return out;
}

struct PSF_font default_font;
struct framebuffer *current_framebuffer;
void graphics_init(struct framebuffer *f)
{
    default_font = parse_font(_psfu_font);
    current_framebuffer = f;
}

void framebuffer_draw_char(struct framebuffer *f, char c, size_t x, size_t y)
{
    struct PSF_font_header header = default_font.header;
    u8 *char_start = (header.bytesperglyph * c) + default_font.fontdata;

    for (size_t y_pos = y; y_pos < y + header.height; y_pos++)
    {
        for (size_t x_pos = x; x_pos < x + header.width; x_pos++)
        {
            if (char_start[y_pos - y] & (1 << (7 - (x_pos - x))))
            {
                framebuffer_put_pixel(f, 0xFFFFFF, (x_pos + 4), y_pos);
            }
            else
            {
            }
        }
    }
}

void framebuffer_pixel_write(struct framebuffer *f, u8 *pixel, int color)
{
    pixel[0] = color & 255;         // BLUE
    pixel[1] = (color >> 8) & 255;  // GREEN
    pixel[2] = (color >> 16) & 255; // RED
}

void framebuffer_put_pixel(struct framebuffer *f, int color, int x, int y)
{
    size_t location = y * f->pitch + x * f->pixel_width;

    u8 *pixel = f->buffer + location;

    framebuffer_pixel_write(f, pixel, color);
}

void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color)
{
    size_t location = y * f->pitch + x * f->pixel_width;

    for (size_t y_pos = y; y_pos < height + y; y_pos++)
    {
        for (size_t x_pos = x; x_pos < width + x; x_pos++)
        {
            u8 *pixel = f->buffer + location;
            framebuffer_pixel_write(f, pixel, color);
            location += f->pixel_width;
        }
        location = y_pos * f->pitch + x * f->pixel_width;
    }
}

/* TODO: should be inline */
struct framebuffer *get_current_framebuffer()
{
    return current_framebuffer;
}
