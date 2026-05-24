#include "lib/log.h"
#include <err_ptr.h>
#include <errno.h>
#include <lib/font.h>
#include <lib/framebuffer.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void framebuffer_draw_char(struct framebuffer *fb, struct PSF_font *font,
                           char c, size_t x, size_t y)
{
    struct PSF_header header = font->header;
    uint8_t *char_start = (header.bytes_per_glyph * c) + font->content;

    for (size_t y_pos = y; y_pos < y + header.height; y_pos++)
    {
        for (size_t x_pos = x; x_pos < x + header.width; x_pos++)
        {
            if (char_start[y_pos - y] & (1 << (7 - (x_pos - x))))
            {

                framebuffer_put_pixel(fb, 0xFFFFFF, x_pos, y_pos);
            }
            else
            {
                /* client_draw_pixel(client, x + 4, y, 0); */
            }
        }
    }
}

void framebuffer_draw_bytes(struct framebuffer *fb, struct PSF_font *font,
                            const char *str, size_t len, size_t x, size_t y)
{
    for (size_t i = 0; i < len; i++)
    {
        framebuffer_draw_char(fb, font, str[i], x + (i * font->header.width),
                              y);
    }
}

void framebuffer_draw_string(struct framebuffer *fb, struct PSF_font *font,
                             const char *str, size_t x, size_t y)
{
    for (size_t i = 0; i < strlen(str); i++)
    {
        framebuffer_draw_char(fb, font, str[i], x + (i * font->header.width),
                              y);
    }
}

#define PSF_FONT_MAGIC 0x864ab572
struct PSF_font *font_parse(u8 *buf)
{
    struct PSF_font *font = malloc(sizeof(struct PSF_font));
    font->header = *(struct PSF_header *)(buf);

    if (font->header.magic != PSF_FONT_MAGIC)
    {
        return ERR_PTR(-EINVAL);
    }

    size_t content_len = font->header.bytes_per_glyph * font->header.num_glyph;
    font->content = buf + sizeof(struct PSF_header);

    return font;
}

struct PSF_font *font_parse_fd(int fd)
{
    struct PSF_font *out = malloc(sizeof(struct PSF_font));

    if (fd < 0)
    {
        return ERR_PTR(-EBADF);
    }

    size_t read_size = read(fd, &out->header, sizeof(struct PSF_header));

    if (read_size != sizeof(struct PSF_header))
    {
        return ERR_PTR(-EWRONGSIZE);
    }

    if (out->header.magic != PSF_FONT_MAGIC)
    {
        return ERR_PTR(-EINVAL);
    }

    size_t content_len = out->header.bytes_per_glyph * out->header.num_glyph;
    out->content = malloc(content_len);
    read(fd, out->content, content_len); /* the file offset should be correct */

    return out;
}
