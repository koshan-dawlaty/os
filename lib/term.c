#include "lib/assert.h"
#include <lib/array.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/term.h>
#include <lib/utils.h>

void terminal_set_pos(struct terminal *term, size_t new_x, size_t new_y)
{
    struct array *line =
        array_get(&term->characters, new_y, sizeof(struct array));

    if (term->x < line->len)
    {
        term->prev_x = term->x;
    }
    else
    {
        term->prev_x = line->len;
    }

    term->prev_y = term->y;

    term->x = new_x;
    term->y = new_y;
}

void terminal_draw_charat(char c, size_t x, size_t y, struct terminal *term)
{

    /* if (y < term->top_line || */
    /*     y >= term->operations.get_height() + term->top_line) */
    /* { */
    /*     logd("terminal_draw_charat position rejected:%d, %d, %d", y, */
    /*          term->top_line, term->operations.get_height()); */
    /*     return; */
    /* } */

    bool is_cursor = false;

    if (x == term->x && y == term->y)
    {
        is_cursor = true;
        if (term->prev_x == term->x && term->prev_y == term->y)
        {
            /* cursor was already drawn here, so return */
            return;
            /* logd("drawing at:%d, %d", term->prev_x, term->prev_y); */
        }
        else
        {
            struct array *prev_line = array_get(&term->characters, term->prev_y,
                                                sizeof(struct array));
            char c2;
            if (term->prev_x >= prev_line->len)
            {
                c2 = ' ';
            }
            else
            {
                c2 = terminal_getchar(*term, term->prev_x, term->prev_y);
            }

            term->operations.draw_char_at(
                c2, term->prev_x, ABS((int)term->prev_y - (int)term->top_line),
                false);
        }
    }

    struct array *line = array_get(&term->characters, y, sizeof(struct array));
    if (x >= line->len)
    {
        c = ' ';
    }
    term->operations.draw_char_at(c, x, ABS((int)y - (int)term->top_line),
                                  is_cursor);
}

static inline void terminal_redraw_char(struct terminal *term, size_t x,
                                        size_t y)
{
    terminal_draw_charat(terminal_getchar(*term, x, y), x, y, term);
}

void terminal_update_line(struct terminal *term, size_t y)
{

    /* logd("redrawing row %d", ABS((int)y - (int)term->top_line)); */
    for (size_t i = 0; i < term->operations.get_width(); i++)
    {
        term->operations.draw_char_at(' ', i, ABS((int)y - (int)term->top_line),
                                      false);
    }
    struct array *line = array_get(&term->characters, y, sizeof(struct array));
    for (size_t i = 0; i < line->len; i++)
    {
        terminal_redraw_char(term, i, y);
    }
}

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
        for (size_t i = 0; i < line->len; i++)
        {
            char c = ((char *)line->data)[i];
            /* terminal_draw_charat(c, i, y + term->top_line, term); */
        }
    }
}

void term_scroll_up(struct terminal *term)
{
    if (term->top_line < 1)
    {
        return;
    }

    term->top_line--;
    size_t height = term->operations.get_height();
    size_t min_height =
        MIN(height, ARRAY_LEN(term->characters, sizeof(struct array)));

    for (size_t i = term->top_line; i < min_height + term->top_line + 1; i++)
    {
        terminal_update_line(term, i);
    }
    /* terminal_redraw_char(term, term->x, term->y); */
}

void term_scroll_down(struct terminal *term)
{
    term->top_line++;
    size_t min_height = MIN(term->operations.get_height(),
                            ARRAY_LEN(term->characters, sizeof(struct array)));

    logd("loop %d:%d", term->top_line - 1, min_height + term->top_line);
    for (size_t i = term->top_line - 1; i < min_height + term->top_line; i++)
    {
        /* terminal_draw_line(term, i, i + 1); */
        logd("drawing line at %d", i);
        terminal_update_line(term, i);
    }
    terminal_redraw_char(term, term->x, term->y);
}

void terminal_add_newline(struct terminal *term)
{

    struct array new_line = ARRAY_INIT();
    array_add(&term->characters, &new_line, sizeof(struct array));
    /* term->operations.draw_char_at(' ', term->x, term->y, */
    /*                               false); /1* clear old cursor *1/ */
    term->y++;
    term->x = 0;

    size_t height = term->operations.get_height();

    if (term->y - term->top_line > height)
    {
        term_scroll_down(term);
        /* term->top_line++; */

        for (size_t i = 0; i < term->operations.get_width(); i++)
        {
            /* clear bottom line */
            /* term->operations.draw_char_at(' ', i, min_height, false); */
            terminal_draw_charat(' ', i, term->y, term);
        }
    }
    else
    {
        terminal_draw_charat(terminal_getchar(*term, term->x, term->y), term->x,
                             term->y, term); /* add new cursor */
    }
}

void terminal_render_newline(struct terminal *term)
{
    size_t old_x = term->x, old_y = term->y;
    char c2 = terminal_getchar(*term, term->x, term->y);

    terminal_add_newline(term);
    terminal_draw_charat(c2, old_x, old_y, term);
}

void terminal_addchar(char c, struct terminal *term)
{
    struct array *line =
        array_get(&term->characters,
                  ARRAY_LEN(term->characters, sizeof(struct array)) - 1,
                  sizeof(struct array));
    size_t width = term->operations.get_width();
    size_t height = term->operations.get_height();

    struct array *a =
        array_get(&term->characters, term->y, sizeof(struct array));
    switch (c)
    {
    case '\n':;
        terminal_render_newline(term);
        break;
    case '\b':;
        if (term->x > 1)
        {

            if (term->x >= a->len)
            {
                terminal_set_pos(term, term->x - 1, term->y);
                array_pop_end(a, 1);
                char c = terminal_getchar(*term, term->x, term->y);
                terminal_draw_charat(c, term->x, term->y, term);
                terminal_draw_charat(' ', term->x + 1, term->y, term);
                break;
            }

            char removed = array_pop_middle(a, term->x);

            term->x -= 1;

            char c = *(char *)array_get(a, term->x, 1);

            terminal_draw_charat(c, term->x, term->y, term);

            for (size_t i = term->x + 1; i < a->len; i++)
            {
                char c2 = *(char *)array_get(a, i, 1);

                terminal_draw_charat(c2, i, term->y, term);
            }
            terminal_draw_charat(' ', a->len, term->y, term);
        }
        else if (term->x > 0)
        {
            term->x = 0;
            array_pop(a, 1);
            terminal_draw_charat(' ', 0, term->y, term);
        }
        else
        {
            /* TODO: remove line */
        }
        break;
    default:;

        array_insert(a, term->x, &c, 1);
        terminal_set_pos(term, term->x + 1, term->y);

        size_t old_x = term->x;
        if (old_x > 0)
        {
            old_x = term->x - 1;
        }

        terminal_draw_charat(terminal_getchar(*term, old_x, term->y), old_x,
                             term->y, term);
        for (size_t i = term->x; i < a->len; i++)
        {
            terminal_draw_charat(((char *)a->data)[i], i, term->y, term);
        }
        if (term->x >= a->len)
        {
            terminal_draw_charat(' ', term->x, term->y, term);
        }

        /* logd("now:%s", a->data); */
        /* array_add(a, &c, 1); */
        /* terminal_draw_charat(c, term->x, term->y, term); */
        /* term->operations.draw_char_at(c, term->x, term->y, false); */

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
    size_t num_lines = (term->characters.len / sizeof(struct array));

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
        size_t old_x = term->x, old_y = term->y;
        terminal_set_pos(term, 0, y);
        char c = terminal_getchar(*term, old_x, old_y);
        terminal_draw_charat(c, old_x, old_y, term);

        terminal_redraw_char(term, term->x, term->y);

        return;
    }

    if (x <= 0)
    {
        x = 0;
    }
    else if (x >= line->len)
    {
        x = line->len; /* let cursor go beyond end of line */
    }

    /* terminal_draw_line(term, y, y); */
    /* logd("drawing at:%d, %d, len:%d", term->x, term->y, line->len);
     */
    char c = terminal_getchar(*term, term->x, term->y);
    terminal_draw_charat(c, term->x, term->y, term);

    terminal_set_pos(term, x, y);

    c = ((char *)line->data)[x];
    terminal_draw_charat(c, term->x, term->y, term);
}
