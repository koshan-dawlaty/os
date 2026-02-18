#include "wm_lib.h"
#include <compiler.h>
#include <fcntl.h>
#include <init.h>
#include <sched.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <types.h>
#include <unistd.h>

int pipe(int fds[2])
{
    return syscall_wrapper_asm(SYS_pipe, (ptr)fds, 0, 0, 0, 0);
}

int execv_stdio(const char *path, char *const argv[], int in, int out)
{
    return syscall_wrapper_asm(SYS_execv_stdio, (ptr)path, (ptr)argv, in, out,
                               0);
}

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
    int fd = open(path, O_RDONLY);
    printf("%d\n", fd);
    pread(fd, header, sizeof(struct tga_header), 0);

    printf("w:%d h:%d, bpp:%d, %d\n", header->w, header->h, header->bpp,
           header->magic1);

    size_t data_size = header->h * header->w * (header->bpp / 8);
    printf("size:%d\n", data_size);
    void *data = malloc(data_size);
    printf("%p\n", data);

    pread(fd, data, data_size, sizeof(struct tga_header));

    return data;
}

int main()
{

    init_libc();

    struct wm_client client = wm_client_connect();

    int font_fd = open("/font.psfu", O_RDONLY);
    struct PSF_font *font = font_parse(font_fd);
    /* printf("%d\n", font->header.bytes_per_glyph); */

    if (font == NULL)
    {
        printf("font was invalid\n");
        exit(1);
    }

    /* memset(framebuffer, 'a', 10); */

    int color = 0xFFFF00;
    switch (client.socket)
    {
    case 1:
        color = 0xFF0000;
        break;
    case 2:
        color = 0x00FF00;
        break;
    case 3:
        color = 0xFF;
    }

    /* memset(w.buffer, 127, win.width * win.height * 4); */
    size_t y;
    for (y = 0; y < client.win.h; y++)
    {
        for (size_t x = 0; x < client.win.w; x++)
        {
            client_draw_pixel(&client.win, x, y, color);
        }
    }

    /* int stdout_pipe[2]; */
    /* pipe(stdout_pipe); */

    /* int stdin_pipe[2]; */
    /* pipe(stdin_pipe); */
    /* /1* execv("/ls", (char *[]){"ls", NULL}); *1/ */
    /* /1* execv("/ksh", (char *[]){"ksh", NULL}); *1/ */
    /* execv_stdio("/ksh", (char *[]){"ksh", NULL}, stdin_pipe[0],
     * stdout_pipe[1]); */
    /* /1* execv_stdio("/ls", (char *[]){"ls", NULL}, 0, 1); *1/ */

    /* size_t len = 100; */
    /* char *buf = malloc(len); */
    /* read(stdout_pipe[0], buf, len); */
    /* printf("read:%s\n", buf); */

    /* const char *str = "\neno tac"; */
    /* write(stdin_pipe[1], str, strlen(str)); */

    /* read(stdout_pipe[0], buf, len); */
    /* printf("read:%s\n", buf); */

    struct tga_header header;
    u8 *img_data = parse_tga_image("osdev.tga", &header);

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

            client_draw_pixel(&client.win, x + 100, y,
                              *((u32 *)(img_data + index)));
        }
    }

    int pos = 10;
    size_t x = 1;
    while (1)
    {
        wm_render(client);

        char c = wm_get_key(client);
        client_draw_char(&client.win, font, c, x, 150);
        x += font->header.width;
    }

    exit(0);
}
