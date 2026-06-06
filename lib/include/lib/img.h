#ifndef LIB_IMG_H
#define LIB_IMG_H

#include <lib/framebuffer.h>
#include <types.h>

struct tga_header
{
    u8 magic1;             // must be zero
    u8 colormap;           // must be zero
    u8 encoding;           // must be 2
    u16 cmaporig, cmaplen; // must be zero
    u8 cmapent;            // must be zero
    u16 x;                 // must be zero
    u16 y;                 // image's height
    u16 w;                 // image's height
    u16 h;                 // image's width
    u8 bpp;                // must be 32
    u8 pixeltype;          // must be 40
} __packed;

struct image
{
    struct tga_header header;
    void *data;
};

void *tga_image_parse(const char *path, struct tga_header *header);
void image_parse(const char *path, struct image *img);
void framebuffer_draw_img(struct framebuffer *fb, size_t x, size_t y,
                          struct image img);
void framebuffer_draw_img_scale(struct framebuffer *fb, size_t x_pos,
                                size_t y_pos, double scale, struct image img);

void framebuffer_draw_img_tga(struct framebuffer *fb, void *data,
                              struct tga_header header);

#endif
