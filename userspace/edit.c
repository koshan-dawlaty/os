#include "container_of.h"
#include "errno.h"
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
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <types.h>
#include <unistd.h>
#include <vfs.h>

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

    FILE *file;
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

void set_filename(struct editor_buffer *buffer)
{
    framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                          font->header.height * 2, 0);
    framebuffer_draw_string(&client.fb, font, "filename:", 0, 0);
    wm_render(client);

    struct array filename = ARRAY_INIT();
    struct wm_message message;
    size_t x = 0;
    while (true)
    {

        wm_get_message(&client, &message);
        if (message.header.type == WM_MESSAGE_KEY)
        {
            char c = *(char *)message.message_data;
            if (c == '\n')
            {
                break;
            }

            array_add(&filename, &c, 1);
            framebuffer_draw_char(&client.fb, font, c, x * font->header.width,
                                  font->header.height);
            wm_render(client);
            x++;
        }
    }
    char zero = 0;
    array_add(&filename, &zero, 1);
    char *name = array_free_ptr(filename);
    logd("got filename:%s", name);
    buffer->file = fopen(name, "c");
}

void insert_char(char c, struct editor_buffer *buffer)
{
    if (c == '\b')
    {
        editor_buffer_delete_cursor(buffer);

        framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                              client.fb.height, 0);
        editor_buffer_redraw(buffer, &client.fb, 0, 0);
    }
    else if (c == '\n')
    {
        editor_buffer_insert_newline_cursor(buffer);
        framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                              client.fb.height, 0);
        editor_buffer_redraw(buffer, &client.fb, 0, 0);
    }
    else
    {
        editor_buffer_insert_cursor(buffer, c);

        framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                              client.fb.height, 0);
        editor_buffer_redraw(buffer, &client.fb, 0, 0);
    }
}

void write_to_file(struct editor_buffer *buffer)
{

    while (buffer->file == NULL)
    {
        set_filename(buffer);
    }

    lseek(buffer->file->fd, 0, SEEK_SET);
    struct list *pos;
    list_for_each(pos, &buffer->rows_head)
    {
        struct editor_row *row = containerof(pos, struct editor_row, list);
        if (row->characters.data != NULL)
        {
            logd("writing:%s", row->characters.data);
            fwrite(row->characters.data, row->characters.len, 1, buffer->file);
        }
        char newline = '\n';
        fwrite(&newline, 1, 1, buffer->file);
    }
}

void handle_args(int argc, char **argv, struct editor_buffer *buffer)
{
    if (argc > 1)
    {
        buffer->file = fopen(argv[1], "c");
        if (buffer->file != NULL)
        {
            size_t read_size = 10;
            char *buf = malloc(read_size);
            bzero(buf, read_size);
            size_t bytes_read = 0;
            bool is_first_read = true;
            do
            {
                bytes_read = fread(buf, read_size, 1, buffer->file);
                if (is_first_read)
                {
                    /* TODO: maybe put this check in another file? */
                    if ((buf[0] == 0x7F && buf[1] == 'E' && buf[2] == 'L' &&
                         buf[3] == 'F') ||
                        /* this is for the tga image format, it will probably
                           cause issues in the future */
                        (buf[0] == 0 && buf[1] == 0))
                    {
                        logw("tried to open file %s that has invalid type",
                             argv[1]);
                        /* this would result in the binary being overwritten if
                         * the file is written to, which is maybe what the user
                         * wants */
                        goto free_buffer;
                    }
                    is_first_read = false;
                }
                for (size_t i = 0; i < bytes_read; i++)
                {
                    insert_char(buf[i], buffer);
                }
            } while (bytes_read == read_size);
            wm_render(client);
        free_buffer:
            free(buf);
        }
    }
}

int main(int argc, char **argv)
{

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

    /* struct editor_buffer buffer = {0, 0, .top_line = 0, */
    /*                                .num_lines = */
    /*                                    client.fb.height /
     * font->header.height}; */
    struct editor_buffer buffer = {0, 0, .top_line = 0, .file = NULL,
                                   .num_lines =
                                       client.fb.height / font->header.height};

    list_init_head(&buffer.rows_head);
    struct editor_row *row = editor_row_alloc();
    list_push(&row->list, &buffer.rows_head);
    buffer.current_row = row;
    handle_args(argc, argv, &buffer);
    /* array_push(&row->characters, "abc", 3); */

    framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width, client.fb.height,
                          0);
    editor_buffer_redraw(&buffer, &client.fb, 0, 0);
    wm_render(client);

    bool insert_mode = true;
    while (wm_is_alive(&client))
    {
        struct wm_message message;
        wm_get_message(&client, &message);
        continue;

        switch (message.header.type)
        {
        case WM_MESSAGE_KEY:
        {

            char c = *(char *)message.message_data;
            if (insert_mode)
            {
                if (c == 27)
                {
                    insert_mode = false;
                }
                else
                {
                    insert_char(c, &buffer);
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
                case 'W':
                    set_filename(&buffer);
                    break;
                }

                framebuffer_draw_rect(&client.fb, 0, 0, client.fb.width,
                                      client.fb.height, 0);
                editor_buffer_redraw(&buffer, &client.fb, 0, 0);

                wm_render(client);
            }
            break;
        }
        default:;
        }
    }

    close(client.socket);

    exit(0);
}
