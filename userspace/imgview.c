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

    struct tga_header header;

    u8 *img_data = tga_image_parse(argv[1], &header);

    struct wm_client client = wm_client_connect(header.h, header.w);

    framebuffer_draw_img(&client.fb, img_data, header);

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
