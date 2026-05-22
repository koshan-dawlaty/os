#include "userspace/mouse.h"
#include "wm_lib.h"
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/log.h>
#include <sched.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <types.h>
#include <unistd.h>

enum explorer_filetype
{
    EXPLORER_TEXT,
    EXPLORER_IMAGE,
    EXPLORER_EXECUTABLE,
    EXPLORER_INVALID,
};
enum explorer_filetype get_filetype(int fd)
{
    char *buf = malloc(4);
    int result = read(fd, buf, 4);
    if (result < 0)
    {
        return EXPLORER_INVALID;
    }
    if (result != 4)
    {
        return EXPLORER_TEXT;
    }

    /* the .tga format does not have a magic number, but the first two bytes are
     * always zero */
    if (buf[0] == 0 &&
        buf[1] == 0) /* TODO: this will definitly result in issues, but it
                        shouldn't conflict with any text files */
    {
        return EXPLORER_IMAGE;
    }

    if (buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' && buf[3] == 'F')
    {
        return EXPLORER_EXECUTABLE;
    }

    return EXPLORER_TEXT;
}

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

struct array read_dir_path(int fd)
{
    DIR *d = fdopendir(fd);
    if (d == NULL)
    {
        PANIC("error when reading dir:%s", strerror(errno));
    }

    struct array ents = ARRAY_INIT();
    struct dirent *e = readdir(d);
    while (e != NULL)
    {
        array_add(&ents, e, sizeof(struct dirent));
        logd("%s", e->d_name);
        e = readdir(d);
    }
    closedir(d);
    return ents;
}

void line_clicked(size_t pos, struct array *ents, struct wm_client client)
{

    struct dirent *e = array_get(ents, pos, sizeof(struct dirent));
    /* TODO: this should use stat() */
    int fd = open(e->d_name, O_RDONLY);
    if (fd < 0 && errno != EISDIR)
    {
        logd("%d", errno);
        PANIC("open error:%s", strerror(errno));
    }
    else if (errno == EISDIR)
    {
        chdir(e->d_name);
        close(fd);
        fd = open(".", O_DIRECTORY);
        struct array new_ents = read_dir_path(fd);
        array_free(ents);
        *ents = new_ents;
    }
    else
    {
        enum explorer_filetype type = get_filetype(fd);

        switch (type)
        {
        case EXPLORER_TEXT:
            /* TODO: display the text */

            client_draw_rect(client, 0, 0, client.fb.width, client.fb.height,
                             0);
            lseek(fd, 0, SEEK_SET);
            size_t x = 0;
            size_t y = 0;
            size_t read_size = 100;
            char *buf = malloc(read_size);
            int bytes_read = read(fd, buf, read_size);
            do

            {
                for (size_t i = 0; i < bytes_read; i++)
                {
                    if (buf[i] == '\n')
                    {
                        y += font->header.height;
                        x = 0;
                    }
                    else
                    {
                        client_draw_char(client, font, buf[i], x, y);
                        x += font->header.width;
                    }
                }
            } while ((bytes_read = read(fd, buf, read_size)) == read_size);
            wm_render(client);

            struct wm_message message = {.header = WM_MESSAGE_INVALID};
            while (message.header.type != WM_MESSAGE_KEY)
            {
                wm_get_message(client, &message);
            }

            break;
        case EXPLORER_IMAGE:
            execv("/bin/imgview", (char *[]){"/bin/imgview", e->d_name, NULL});
            break;
        case EXPLORER_EXECUTABLE:
            execv(e->d_name, (char *[]){e->d_name, NULL});
            break;
        case EXPLORER_INVALID:
            PANIC("invalid filetype from fd:%d", fd);
            break;
        }

        close(fd);
    }
}

int mouse_y = 0;

int main(int argc, char **argv)
{

    init_libc();

    /* sleep(400); */

    struct wm_client client = wm_client_connect(600, 200);
    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse(font_fd);

    sched_yield();
    wm_render(client);

    size_t pos = 0;

    int fd = open("/", O_DIRECTORY);
    struct array ents = read_dir_path(fd);
    render(client, &ents, pos);

    while (1)
    {
        struct wm_message message;
        wm_get_message(client, &message);
        switch (message.header.type)
        {
        case WM_MESSAGE_MOUSE:;
            struct mouse_event *ev = message.message_data;
            mouse_y += ev->y_change;

            if (mouse_y >= 0 &&
                mouse_y < font->header.height *
                              ARRAY_LEN(ents, sizeof(struct dirent)))
            {
                pos = mouse_y / font->header.height;
            }
            /* if (mouse_y / font->header.height != pos) */
            /* { */
            /*     pos = mouse_y / font->header.height; */
            /* } */
            /* if (ev->y_change < 0) */
            /* { */
            /*     pos--; */
            /* } */
            /* else if (ev->y_change > 0) */
            /* { */
            /*     pos++; */
            /* } */

            if (ev->buttons.left_click)
            {
                line_clicked(pos, &ents, client);
                pos = 0;
            }

            break;

        case WM_MESSAGE_KEY:;
            char c = *(char *)message.message_data;

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
                line_clicked(pos, &ents, client);
                pos = 0;
                break;
            }
            break;
        }

        /* could only update area that was changed here, but it probably does
         * not matter */
        render(client, &ents, pos);
    }

    close(client.socket);

    exit(0);
}
