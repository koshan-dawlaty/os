#include "lib/framebuffer.h"
#include "lib/log.h"
#include "sys/types.h"
#include "unistd.h"
#include "wm_lib.h"
#include <dirent.h>
#include <errno.h>
#include <init.h>
#include <lib/array.h>
#include <lib/assert.h>
#include <lib/font.h>
#include <string.h>

struct wm_client client;
struct PSF_font *font;

struct array read_dir_array(DIR *d)
{

    struct array ents = ARRAY_INIT();
    struct dirent *e = readdir(d);
    while (e != NULL)
    {
        array_add(&ents, e, sizeof(struct dirent));
        e = readdir(d);
    }
    closedir(d);
    return ents;
}

#define BIN_DIR "/bin"
#define NUM_LINES 10

int main()
{
    init_libc();

    DIR *bin_dir = opendir(BIN_DIR);
    if (bin_dir == NULL)
    {
        PANIC("error when reading dir:%s", strerror(errno));
    }

    size_t max = 0;
    struct array ents = read_dir_array(bin_dir);
    for (size_t i = 0; i < ARRAY_LEN(ents, sizeof(struct dirent)); i++)
    {
        struct dirent *d = array_get(&ents, i, sizeof(struct dirent));
        if (strlen(d->d_name) > max)
        {
            max = strlen(d->d_name);
        }
    }

    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);

    /* client = wm_client_connect_focus(font->header.height * */
    /*                                      ARRAY_LEN(ents, sizeof(struct
     * dirent)), */
    /*                                  font->header.width * max); */

    client = wm_client_connect_focus(font->header.height * NUM_LINES,
                                     font->header.width * max);

    struct array search = ARRAY_INIT();

    while (true)
    {
        wm_render(client);
        struct wm_message message;
        wm_get_message(&client, &message);
        switch (message.header.type)
        {
        case WM_MESSAGE_KEY:;
            char c = *(char *)(message.message_data);

            if (c == '\n')
            {
                for (size_t i = 0; i < ARRAY_LEN(ents, sizeof(struct dirent));
                     i++)
                {
                    struct dirent *ent =
                        array_get(&ents, i, sizeof(struct dirent));

                    if (!strncmp(ent->d_name, search.data, search.len))
                    {

                        char *str = malloc(strlen(ent->d_name) +
                                           strlen(BIN_DIR) + 1 /* slash */ + 1);
                        stpcpy(stpcpy(stpcpy(str, BIN_DIR), "/"), ent->d_name);
                        execv(str, (char *[]){str, NULL});
                    }
                }
                array_free(&search);

                goto done;
            }
            size_t y = 0;
            framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                                  client.fb.height, 0);

            if (c == '\b')
            {
                if (search.len >= 1)
                {
                    array_pop_end(&search, 1);
                }
            }
            else
            {
                array_add(&search, &c, 1);
            }

            framebuffer_draw_bytes(&client.fb, font, search.data, search.len, 0,
                                   y);
            y += font->header.height;

            for (size_t i = 0; i < ARRAY_LEN(ents, sizeof(struct dirent)); i++)
            {
                struct dirent *ent = array_get(&ents, i, sizeof(struct dirent));
                if (!strncmp(ent->d_name, search.data, search.len))
                {
                    framebuffer_draw_string(&client.fb, font, ent->d_name, 0,
                                            y);
                    y += font->header.height;
                }
            }

            break;
        }
    }
done:

    /* TODO: exit() does not call the close operation on sockets */
    close(client.socket);
    close(client.key_socket);

    logd("exiting");
    exit(0);
}
