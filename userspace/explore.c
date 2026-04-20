#include "wm_lib.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/log.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <types.h>
#include <unistd.h>

struct PSF_font *font;
void render(struct wm_client c, struct array *ents, size_t cursor_pos)
{
    client_draw_rect(c, 0, 0, c.fb.width, c.fb.height, 0);

    client_draw_rect(c, 0, cursor_pos * font->header.height, c.fb.width,
                     font->header.height, 0xFF0000);

    size_t x = 0;
    size_t y = 0;
    for (size_t i = 0; i < (ents->len / sizeof(struct dirent)); i++)
    {
        struct dirent *ent = array_get(ents, i, sizeof(struct dirent));

        char *name = ent->d_name;
        while (*name != 0)
        {
            client_draw_char(c, font, *name, x, y);
            x += font->header.width;

            name++;
        }
        x = 0;
        y += font->header.height;
    }

    /* logd("%p", e); */

    wm_render(c);
}

struct array read_dir_path(const char *path)
{
    DIR *d = opendir(path);
    if (d == NULL)
    {
        loge("error when reading dir:%s", strerror(errno));
    }
    logd("dir:%p", d->fd);

    struct array ents = ARRAY_INIT();
    struct dirent *e = readdir(d);
    while (e != NULL)
    {
        array_add(&ents, e, sizeof(struct dirent));
        e = readdir(d);
    }
    /* TODO: closedir() */
    return ents;
}

int main(int argc, char **argv)
{

    init_libc();

    /* sleep(400); */

    struct wm_client client = wm_client_connect(600, 600);
    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse(font_fd);

    sched_yield();
    wm_render(client);

    size_t pos = 0;

    struct array ents = read_dir_path("/");
    render(client, &ents, pos);

    while (1)
    {
        char c = wm_get_key(client);

        switch (c)
        {
        case 'j':
            if (pos < ARRAY_LEN(ents, sizeof(struct dirent)) - 1)
            {
                pos++;
            }
            break;
        case 'k':
            if (pos != 0)
            {
                pos--;
            }
            break;
        case '\n':;
            struct dirent *e = array_get(&ents, pos, sizeof(struct dirent));
            chdir(e->d_name);
            struct array new_ents = read_dir_path(".");
            array_free(&ents);
            ents = new_ents;
            pos = 0;

            break;
        }

        /* could only update area that was changed here, but it probably does
         * not matter */
        render(client, &ents, pos);
    }

    close(client.socket);

    exit(0);
}
