#include "lib/utils.h"
#include <lib/list.h>
#include <lib/term.h>

static void terminal_draw_line(struct terminal *term, size_t index, size_t y)
{
    struct array *line =
        array_get(&term->characters, index, sizeof(struct array));
    /* logd("%d len of %s:%d", y, line->data, line->len); */
    for (size_t i = 0; i < term->operations.get_width(); i++)
    {
        term->operations.draw_char_at(' ', i, y);
    }
    for (size_t i = 0; i < line->len - 1; i++)
    {
        char c = ((char *)line->data)[i];
        term->operations.draw_char_at(c, i, y);
    }
}

void terminal_add_newline(struct terminal *term)
{
    struct array new_line = ARRAY_INIT();
    array_add(&term->characters, &new_line, sizeof(struct array));
    term->y++;
    term->x = 0;
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
            term->operations.draw_char_at(' ', i, min_height);
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
    array_add(line, &c, 1);
    size_t width = term->operations.get_width();
    size_t height = term->operations.get_height();
    switch (c)
    {
    case '\n':

        terminal_add_newline(term);
        break;
    case '\b':
        term->x--;
        term->operations.draw_char_at(' ', term->x, term->y);
        break;
    default:
        term->operations.draw_char_at(c, term->x, term->y);
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
