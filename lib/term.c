#include "lib/array.h"
#include "lib/log.h"
#include "lib/utils.h"
#include <lib/list.h>
#include <lib/term.h>

void terminal_draw_line(struct terminal *term, size_t index, size_t y)
{
    struct array *line =
        array_get(&term->characters, index, sizeof(struct array));

    /* logd("%d len of %s:%d", y, line->data, line->len); */
    for (size_t i = 0; i < term->operations.get_width(); i++)
    {
        term->operations.draw_char_at(' ', i, y, false);
    }
    if (line->len != 0)
    {
        for (size_t i = 0; i < line->len - 1; i++)
        {
            char c = ((char *)line->data)[i];
            term->operations.draw_char_at(c, i, y, false);
        }
    }
}

void terminal_add_newline(struct terminal *term)
{
    struct array new_line = ARRAY_INIT();
    array_add(&term->characters, &new_line, sizeof(struct array));
    term->operations.draw_char_at(' ', term->x, term->y,
                                  false); /* clear old cursor */
    term->y++;
    term->x = 0;

    term->operations.draw_char_at(' ', term->x, term->y,
                                  true); /* add new cursor */

    size_t height = term->operations.get_height();

    if (term->y > height)
    {
        term->y -= 1;
        size_t min_height =
            MIN(height, ARRAY_LEN(term->characters, sizeof(struct array)));

        for (size_t i = term->top_line; i < min_height + term->top_line - 1;
             i++)
        {
            terminal_draw_line(term, i, i - term->top_line + 1);
        }

        for (size_t i = 0; i < term->operations.get_width(); i++)
        {
            /* clear bottom line */
            term->operations.draw_char_at(' ', i, min_height, false);
        }

        term->top_line++;
    }
}

void terminal_addchar(char c, struct terminal *term)
{

    struct array *line =
        array_get(&term->characters,
                  ARRAY_LEN(term->characters, sizeof(struct array)) - 1,
                  sizeof(struct array));
    size_t width = term->operations.get_width();
    size_t height = term->operations.get_height();
    switch (c)
    {
    case '\n':
        terminal_add_newline(term);
        break;
    case '\b':
        if (term->x > 0)
        {
            term->x--;
            array_pop_end(line, 1);
            term->operations.draw_char_at(' ', term->x, term->y, true);
            term->operations.draw_char_at(' ', term->x + 1, term->y, false);
        }
        break;
    default:;
        logd("getting line at index:%d, len:%d", term->y,
             ARRAY_LEN(term->characters, sizeof(struct array)));
        struct array *a =
            array_get(&term->characters, term->y, sizeof(struct array));
        logd("array:%p", a);
        char c2 = ' ';
        if (a->data != NULL)
        {
            c2 = *(char *)a->data;
        }
        logd("adding:%c, at:%d", c, term->y);
        array_add(line, &c, 1);
        term->operations.draw_char_at(' ', term->x + 1, term->y, true);
        term->operations.draw_char_at(c, term->x, term->y, false);
        term->x++;

        if (term->x > width)
        {
            terminal_add_newline(term);
        }
        break;
    }
}

void terminal_add(const char *data, size_t len, struct terminal *term)
{
    for (size_t i = 0; i < len; i++)
    {
        terminal_addchar(data[i], term);
    }
}

void terminal_set_cursor(struct terminal *term, int x, int y)
{
    /* logd("%d", term->top_line); */
    size_t num_lines = (term->characters.len / sizeof(struct array));
    logd("before:%d %d num:%d", x, y, num_lines);

    if (y <= 0)
    {
        y = 0;
    }
    else if (y >= (int)num_lines)
    {
        y = (int)num_lines - 1;
    }

    struct array *line = array_get(&term->characters, y, sizeof(struct array));

    if (line->len == 0)
    {
        logd("empty");
        return;
    }

    if (x <= 0)
    {
        x = 0;
    }
    else if (x >= line->len)
    {
        x = line->len - 1;
    }

    logd("after:%d %d, %d, %d line len:%d, %s", x, y, term->x, term->y,
         line->len, line->data);
    /* terminal_draw_line(term, y, y); */
    char c = terminal_getchar(*term, term->x, term->y);
    term->operations.draw_char_at(c, term->x, term->y, false);

    term->x = x;
    term->y = y;

    c = ((char *)line->data)[x];
    term->operations.draw_char_at(c, term->x, term->y, true);
}
