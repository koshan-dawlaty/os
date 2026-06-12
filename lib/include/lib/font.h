#ifndef LIB_FONT_H
#define LIB_FONT_H

#include <compiler.h>
#include <lib/framebuffer.h>

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

struct PSF_font *font_parse(u8 *buf);
struct PSF_font *font_parse_fd(int fd);

void framebuffer_draw_char(struct framebuffer *fb, struct PSF_font *font,
                           char c, size_t x, size_t y);

void framebuffer_draw_string(struct framebuffer *fb, struct PSF_font *font,
                             const char *str, size_t x, size_t y);

void framebuffer_draw_bytes(struct framebuffer *fb, struct PSF_font *font,
                            const char *str, size_t len, size_t x, size_t y);

#endif
