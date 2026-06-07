#include "lib/assert.h"
#include "wm_lib.h"
#include <fcntl.h>
#include <init.h>
#include <lib/img.h>
#include <lib/log.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <types.h>
#include <unistd.h>

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
    const char *path = argv[1];

    struct image img;
    image_parse(path, &img);

    if (img.data == NULL)
    {
        PANIC("invalid image %s\n", path);
    }

    struct wm_client client = wm_client_connect(img.header.h, img.header.w);

    framebuffer_draw_img(&client.fb, 0, 0, img);

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
