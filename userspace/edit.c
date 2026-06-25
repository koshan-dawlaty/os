#include "container_of.h"
#include "lib/array.h"
#include "lib/font.h"
#include "lib/list.h"
#include "wm_lib.h"
#include <err_ptr.h>
#include <fcntl.h>
#include <init.h>
#include <lib/assert.h>
#include <lib/framebuffer.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/term.h>
#include <math.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <types.h>
#include <unistd.h>

struct wm_client client;
struct PSF_font *font;

struct editor_row
{
    struct array characters;

    struct list list;
};
struct editor_buffer
{
    size_t cursor_x, cursor_y;
    /* the topmost line that is visible */
    size_t top_line;
    /* the number of lines that can be viewed, the height the buffer on the
     * screen */
    size_t num_lines;

    struct list rows_head;
    struct editor_row *current_row;
};

struct editor_row *editor_row_alloc()
{
    struct editor_row *out = malloc(sizeof(struct editor_row));
    out->characters = ARRAY_INIT();
    out->list = LIST_INIT();

    return out;
}

void editor_buffer_scroll_up(struct editor_buffer *buffer)
{
    if (buffer->top_line > 0)
    {
        buffer->top_line--;
    }
}

void editor_buffer_set_cursor(struct editor_buffer *buffer, int cursor_x,
                              int cursor_y);

void editor_buffer_redraw(struct editor_buffer *buffer, struct framebuffer *fb,
                          size_t x_pos, size_t y_pos)
{

    framebuffer_draw_rect(fb, buffer->cursor_x * font->header.width,
                          font->header.height *
                              (buffer->cursor_y - buffer->top_line),
                          font->header.width, font->header.height, 0x00FF00);

    struct list *pos;
    size_t y = 0;
    size_t i = 0;
    list_for_each(pos, &buffer->rows_head)
    {
        if (i >= buffer->top_line && i <= buffer->num_lines + buffer->top_line)
        {
            struct editor_row *row = containerof(pos, struct editor_row, list);

            for (size_t x = 0; x < row->characters.len; x++)
            {
                char c = ((char *)row->characters.data)[x];

                size_t real_x = x_pos + (x * font->header.width);
                size_t real_y = y_pos + (y * font->header.height);
                framebuffer_draw_char(fb, font, c, real_x, real_y);
            }
            y++;
        }

        i++;
    }
}

void editor_buffer_delete_cursor(struct editor_buffer *buffer)
{
    if (buffer->cursor_x > 0)
    {
        array_pop_middle(&buffer->current_row->characters, buffer->cursor_x);
        buffer->cursor_x--;
    }
    else if (buffer->cursor_y != 0)
    {
        struct editor_row *prev_row = containerof(
            buffer->current_row->list.prev, struct editor_row, list);
        if (buffer->current_row->characters.len != 0)
        {

            array_add(&prev_row->characters,
                      buffer->current_row->characters.data,
                      buffer->current_row->characters.len);
        }

        list_remove(&buffer->current_row->list);
        buffer->current_row = prev_row;

        /* place the cursor at the start of the old line, so that if more
         * characters are deleted, it won't start deleting the part of the line
         * after the cursor */
        editor_buffer_set_cursor(buffer, prev_row->characters.len,
                                 buffer->cursor_y - 1);
    }
}

void editor_buffer_insert_newline_cursor(struct editor_buffer *buffer)
{
    struct editor_row *new_row = editor_row_alloc();
    list_add_after(&new_row->list, &buffer->current_row->list);

    array_push(&new_row->characters,
               buffer->current_row->characters.data + buffer->cursor_x,
               buffer->current_row->characters.len - buffer->cursor_x);

    for (size_t i = buffer->current_row->characters.len; i > buffer->cursor_x;
         i--)
    {
        /* stupid, because  it should be one function to do this */
        array_pop_middle(&buffer->current_row->characters, i);
    }

    buffer->current_row = new_row;
    editor_buffer_set_cursor(buffer, 0, buffer->cursor_y + 1);
}

void editor_buffer_insert_cursor(struct editor_buffer *buffer, char c)
{
    array_insert(&buffer->current_row->characters, buffer->cursor_x, &c, 1);
    buffer->cursor_x++;
}

void editor_buffer_set_cursor(struct editor_buffer *buffer, int cursor_x,
                              int cursor_y)
{

    if (cursor_x < 0)
    {
        cursor_x = 0;
    }
    if (cursor_y < 0)
    {
        cursor_y = 0;
    }

    /* traversing the list here is slower, in editor_buffer_move_cursor(),
     * maybe use buffer->current_row? */
    struct list *pos;
    size_t y = 0;
    list_for_each(pos, &buffer->rows_head)
    {
        if (y == cursor_y)
        {
            goto found;
        }

        y++;
    }
    cursor_y = y - 1;
    pos = pos->prev;

found:;

    if (cursor_y == buffer->top_line)
    {
        editor_buffer_scroll_up(buffer);
    }
    else if (cursor_y == buffer->num_lines + buffer->top_line)
    {
        buffer->top_line++;
    }

    struct editor_row *row = containerof(pos, struct editor_row, list);
    buffer->current_row = row;
    if (cursor_x > row->characters.len)
    {
        cursor_x = row->characters.len;
    }

    buffer->cursor_x = cursor_x;
    buffer->cursor_y = cursor_y;
}

void editor_buffer_move_cursor(struct editor_buffer *buffer, int x_diff,
                               int y_diff)
{
    editor_buffer_set_cursor(buffer, buffer->cursor_x + x_diff,
                             buffer->cursor_y + y_diff);
}

void write_to_file(struct editor_buffer *buffer)
{
    int fd = open("/one", O_WRONLY);
    struct list *pos;
    list_for_each(pos, &buffer->rows_head)
    {
        struct editor_row *row = containerof(pos, struct editor_row, list);
        if (row->characters.data != NULL)
        {
            logd("writing:%s", row->characters.data);
            write(fd, row->characters.data, row->characters.len);
        }
        char newline = '\n';
        write(fd, &newline, 1);
    }
}

int main()
{

    init_libc();

    int font_fd = open("/font.psfu", O_RDONLY);
    font = font_parse_fd(font_fd);

    if (IS_ERR(font))
    {
        printf("font was invalid, error:%s\n", strerror(GET_ERR(font)));
        exit(1);
    }

    /* characters.y = 0; */
    size_t w = 300, h = 300;
    size_t x = 0, y = 0;
    client = wm_client_connect(h, w);

    wm_render(client);

    /* struct editor_buffer buffer = {0, 0, .top_line = 0, */
    /*                                .num_lines = */
    /*                                    client.fb.height /
     * font->header.height}; */
    struct editor_buffer buffer = {0, 0, .top_line = 0, .num_lines = 4};
    list_init_head(&buffer.rows_head);
    struct editor_row *row = editor_row_alloc();
    list_push(&row->list, &buffer.rows_head);
    buffer.current_row = row;

    bool insert_mode = true;
    while (1)
    {
        struct wm_message message;
        wm_get_message(&client, &message);

        switch (message.header.type)
        {
        case WM_MESSAGE_KEY:;

            char c = *(char *)message.message_data;
            if (insert_mode)
            {
                if (c == 27)
                {
                    insert_mode = false;
                }
                else if (c == '\b')
                {
                    editor_buffer_delete_cursor(&buffer);

                    framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                                          client.fb.height, 0);
                    editor_buffer_redraw(&buffer, &client.fb, 0, 0);
                    wm_render(client);
                }
                else if (c == '\n')
                {
                    editor_buffer_insert_newline_cursor(&buffer);
                    framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                                          client.fb.height, 0);
                    editor_buffer_redraw(&buffer, &client.fb, 0, 0);
                    wm_render(client);
                }
                else
                {
                    editor_buffer_insert_cursor(&buffer, c);

                    framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                                          client.fb.height, 0);
                    editor_buffer_redraw(&buffer, &client.fb, 0, 0);
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
                case 'f':
                    buffer.top_line++;
                    break;
                case 'd':
                    buffer.top_line--;
                    break;
                case 'k':
                    editor_buffer_move_cursor(&buffer, 0, -1);
                    break;
                case 'j':
                    editor_buffer_move_cursor(&buffer, 0, 1);
                    break;
                case 'h':
                    editor_buffer_move_cursor(&buffer, -1, 0);
                    break;
                case 'l':
                    editor_buffer_move_cursor(&buffer, 1, 0);
                    break;
                case 'w':
                    write_to_file(&buffer);
                    break;
                }

                framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                                      client.fb.height, 0);
                editor_buffer_redraw(&buffer, &client.fb, 0, 0);
                wm_render(client);

                wm_render(client);
            }
            break;
        default:;
        }
    }

    close(client.socket);

    exit(0);
}
