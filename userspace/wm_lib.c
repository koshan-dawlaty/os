#include "wm_lib.h"
#include <stdint.h>
#include <stddef.h>

void framebuffer_pixel_write(struct framebuffer *f, uint8_t *pixel, int color)
{
    pixel[0] = color & 255;         // BLUE
    pixel[1] = (color >> 8) & 255;  // GREEN
    pixel[2] = (color >> 16) & 255; // RED
}

void framebuffer_draw_rect(struct framebuffer *f, size_t x, size_t y,
                           size_t width, size_t height, int color)
{
    size_t location = y * f->pitch + x * f->pixel_width;

    for (size_t y_pos = y; y_pos < height + y; y_pos++)
    {
        for (size_t x_pos = x; x_pos < width + x; x_pos++)
        {
            uint8_t *pixel = f->buffer + location;
            framebuffer_pixel_write(f, pixel, color);
            location += f->pixel_width;
        }
        location = y_pos * f->pitch + x * f->pixel_width;
    }
}

struct PSF_header {
    uint32_t magic;
    uint32_t version;
    uint32_t header_size;
    uint32_t flags;
    uint32_t num_glyph;
    uint32_t bytes_per_glyph;
    uint32_t height;
    uint32_t width;
};

struct PSF_font {
    struct PSF_header header;
    uint8_t * content;
};

struct PSF_font * parse_font(int fd) {
    struct PSF_font * out = malloc(sizeof(struct PSF_font));
    
    pread(fd, &out->header, sizeof(struct PSF_header), 0);
    
    size_t content_len = out->header.bytes_per_glyph * out->header.num_glyph;
    out->content = malloc(content_len);
    pread(fd, out->content, content_len, out->header.header_size);
    
    return out;
}
