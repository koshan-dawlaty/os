#include "lib/log.h"
#include "sched.h"
#include "sys/types.h"
#include "wm_lib.h"
#include <fcntl.h>
#include <init.h>
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

/*
 * returns a heap pointer to pixel data
 */
void *parse_tga_image(const char *path, struct tga_header *header)
{
    logd("reading from:%s", path);
    int fd = open(path, O_RDONLY);
    logd("fd:%d", fd);
    pread(fd, header, sizeof(struct tga_header), 0);

    /* printf("w:%d h:%d, bpp:%d, %d\n", header->w, header->h, header->bpp, */
    /*        header->encoding); */

    size_t data_size = header->h * header->w * (header->bpp / 8);
    void *data = calloc(data_size, 1);

    int err = pread(fd, data, data_size, sizeof(struct tga_header));
    if (err < 0)
    {
        printf("bad fd\n");
        exit(1);
    }

    return data;
}

int main(int argc, char **argv)
{

    init_libc();

    /* sleep(400); */
    logd("imgview");

    if (argc < 2)
    {
        loge("no image file\n");
        exit(1);
    }

    struct tga_header header;
    logd("parsing image");
    u8 *img_data = parse_tga_image(argv[1], &header);
    logd("img parsed");

    struct wm_client client = wm_client_connect(header.h, header.w);

    for (size_t y = 0; y < header.h; y++)
    {
        for (size_t x = 0; x < header.w; x++)
        {
            size_t index = (x + (y * header.w)) * (header.bpp / 8);

            u8 *pixel = img_data + index;

            u8 b = pixel[0];
            u8 g = pixel[1];
            u8 r = pixel[2];
            u8 a = pixel[3];

            u32 color = (a << 24) | (r << 16) | (g << 8) | b;

            client_draw_pixel(client, x, y, *((u32 *)(img_data + index)));
        }
    }

    sched_yield();
    wm_render(client);

    while (1)
    {
        struct wm_message message;
        wm_get_message(client, &message);
        /* wm_render(client); */
    }

    close(client.socket);

    exit(0);
}
