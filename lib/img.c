#include <fcntl.h>
#include <lib/framebuffer.h>
#include <lib/log.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>
#include <unistd.h>

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

void *tga_image_parse(const char *path, struct tga_header *header)
{

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return NULL;
    }

    read(fd, header, sizeof(struct tga_header));

    /* printf("w:%d h:%d, bpp:%d, %d\n", header->w, header->h, header->bpp, */
    /*        header->encoding); */

    size_t data_size = header->h * header->w * (header->bpp / 8);
    void *data = calloc(data_size, 1);

    int err =
        read(fd, data,
             data_size); /* should be offset by sizeof(struct tga_header) now */
    if (err < 0)
    {
        printf("bad fd\n");
        exit(1);
    }

    return data;
}

void framebuffer_draw_img(struct framebuffer *fb, void *data,
                          struct tga_header header)
{

    for (size_t y = 0; y < header.h; y++)
    {
        for (size_t x = 0; x < header.w; x++)
        {
            size_t index = (x + (y * header.w)) * (header.bpp / 8);

            u8 *pixel = data + index;

            u8 b = pixel[0];
            u8 g = pixel[1];
            u8 r = pixel[2];
            u8 a = pixel[3];

            u32 color = (a << 24) | (r << 16) | (g << 8) | b;

            /* logd("%p, %d, %d", color, x, y); */

            /* framebuffer_put_pixel(fb, x, y, *((u32 *)(data + index))); */
            framebuffer_put_pixel(fb, color, x, y);
        }
    }
    logd("done");
}
