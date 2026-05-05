#include "wm_lib.h"
#include <err_ptr.h>
#include <fcntl.h>
#include <init.h>
#include <lib/assert.h>
#include <lib/framebuffer.h>
#include <lib/log.h>
#include <lib/term.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <types.h>
#include <unistd.h>

struct wm_client client;
struct PSF_font *font;

size_t wm_terminal_get_width()
{
    return client.fb.width / font->header.width;
}

size_t wm_terminal_get_height()
{
    return 3;
    return (client.fb.height / font->header.height) - 2;
}

u32 bg_color = 0xFF;
u32 cursor_color = 0xFF00FF;

void wm_terminal_draw_char_at(char character, size_t x, size_t y,
                              bool is_cursor)
{

    if (character == -1)
    {
        /* PANIC("invalid character"); */
        logw("invalid character at position:%d, %d", x, y);
        return;
    }

    if (is_cursor)
    {
        client_draw_rect(client, x * font->header.width,
                         y * font->header.height, font->header.width,
                         font->header.height, cursor_color);
    }
    else
    {
        client_draw_rect(client, x * font->header.width,
                         y * font->header.height, font->header.width,
                         font->header.height, bg_color);
    }
    client_draw_char(client, font, character, x * font->header.width,
                     y * font->header.height);
}

struct terminal_operations term_ops = {
    .draw_char_at = wm_terminal_draw_char_at,
    .get_height = wm_terminal_get_height,
    .get_width = wm_terminal_get_width,
};

void write_to_file(struct terminal *characters)
{
    int fd = open("/one", O_WRONLY);
    for (size_t i = 0; i < characters->characters.len / sizeof(struct array);
         i++)
    {
        struct array *line =
            array_get(&characters->characters, i, sizeof(struct array));
        if (line->data != NULL)
        {
            logd("writing:%s", line->data);
            write(fd, line->data, line->len);
        }
        char newline = '\n';
        write(fd, &newline, 1);
    }
}

int main()
{

    init_libc();

    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse(font_fd);

    if (IS_ERR(font))
    {
        printf("font was invalid, error:%s\n", strerror(GET_ERR(font)));
        exit(1);
    }

    struct terminal characters = terminal_init(term_ops);
    /* characters.y = 0; */
    size_t w = 300, h = 300;
    size_t x = 0, y = 0;
    client = wm_client_connect(h, w);

    /* terminal_addchar('\n', &characters); */
    /* terminal_draw_line(&characters, 0, 0); */

    /* terminal_add("abcdef", 6, &characters); */
    /* terminal_draw_line(&characters, 0, 0); */
    terminal_add("a\nb\nc\nd\ne", 7, &characters);

    /* terminal_add("\b", 1, &characters); */
    /* terminal_add("123\b\n", 5, &characters); */
    /* terminal_add("2\n", 2, &characters); */
    /* terminal_add("3", 1, &characters); */

    /* terminal_set_cursor(&characters, 0, 1); */

    wm_render(client);

    bool insert_mode = false;

    while (1)
    {
        char c = wm_get_key(client);
        if (insert_mode)
        {
            if (c == 27)
            {
                insert_mode = false;
            }
            else
            {
                terminal_addchar(c, &characters);
                wm_render(client);
            }
        }
        else
        {
            switch (c)
            {
            case 'i':
                insert_mode = true;
                break;
            case 'a':
                term_scroll_up(&characters);
                break;
            case 's':
                term_scroll_down(&characters);
                break;
            case 'k':
                terminal_move_cursor(&characters, 0, -1);
                break;
            case 'j':
                terminal_move_cursor(&characters, 0, 1);
                break;
            case 'h':
                terminal_move_cursor(&characters, -1, 0);
                break;
            case 'l':
                terminal_move_cursor(&characters, 1, 0);
                break;
            case 'w':
                write_to_file(&characters);
                break;
            }

            wm_render(client);
        }
    }

    close(client.socket);

    exit(0);
}
