#include <fcntl.h>
#include <lib/framebuffer.h>
#include <lib/img.h>
#include <lib/log.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <types.h>
#include <unistd.h>

void *tga_image_parse(const char *path, struct tga_header *header)
{

    int fd = open(path, O_RDONLY);
    if (fd < 0)
    {
        return NULL;
    }

    read(fd, header, sizeof(struct tga_header));

    logd("w:%d h:%d, bpp:%d, %d", header->w, header->h, header->bpp,
         header->encoding);

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

void image_parse(const char *path, struct image *img)
{
    img->data = tga_image_parse(path, &img->header);
}

void framebuffer_draw_img(struct framebuffer *fb, size_t x_pos, size_t y_pos,
                          struct image img)
{

    size_t height = img.header.h, width = img.header.w;

    if (unlikely(x_pos > fb->width || y_pos > fb->height))
    {
        return;
    }

    if (fb->width < x_pos + width)
    {
        width = fb->width - x_pos;
    }

    if (fb->height < y_pos + height)
    {
        height = fb->height - y_pos;
    }

    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            size_t index = (x + (y * img.header.w)) * (img.header.bpp / 8);

            u8 *pixel = img.data + index;

            u8 b = pixel[0];
            u8 g = pixel[1];
            u8 r = pixel[2];
            u8 a = pixel[3];

            u32 color = (a << 24) | (r << 16) | (g << 8) | b;
            framebuffer_put_pixel(fb, color, x + x_pos, y + y_pos);
        }
    }
}

void framebuffer_draw_img_dim_scale(struct framebuffer *fb, size_t x_pos,
                                    size_t y_pos, double w_scale,
                                    double h_scale, struct image img)
{

    for (size_t y = 0; y < img.header.h * h_scale; y++)
    {
        for (size_t x = 0; x < img.header.w * w_scale; x++)
        {
            size_t index = ((size_t)(x * (1 / w_scale)) +
                            ((size_t)(y * (1 / h_scale)) * img.header.w)) *
                           (img.header.bpp / 8);

            u8 *pixel = img.data + index;

            u8 b = pixel[0];
            u8 g = pixel[1];
            u8 r = pixel[2];
            u8 a = pixel[3];

            u32 color = (a << 24) | (r << 16) | (g << 8) | b;
            framebuffer_put_pixel(fb, color, x + x_pos, y + y_pos);
        }
    }
}
void framebuffer_draw_img_scale(struct framebuffer *fb, size_t x_pos,
                                size_t y_pos, double scale, struct image img)
{
    framebuffer_draw_img_dim_scale(fb, x_pos, y_pos, scale, scale, img);
}
