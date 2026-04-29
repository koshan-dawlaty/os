#include <lib/array.h>
#include <lib/list.h>
#include <lib/log.h>
#include <lib/term.h>
#include <lib/utils.h>

void terminal_set_pos(struct terminal *term, size_t new_x, size_t new_y)
{
    term->prev_x = term->x;
    term->prev_y = term->y;

    term->x = new_x;
    term->y = new_y;
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
        for (size_t i = 0; i < line->len - 1; i++)
        {
            char c = ((char *)line->data)[i];
            term->operations.draw_char_at(c, i, y, false);
        }
    }
}

void terminal_draw_charat(char c, size_t x, size_t y, struct terminal *term)
{
    if (x == term->x && y == term->y)
    {
        term->operations.draw_char_at(c, x, y, true);
        if (term->prev_x != term->x || term->prev_y != term->y)
        {
            char c2 = terminal_getchar(*term, term->prev_x, term->prev_y);
            term->operations.draw_char_at(c2, term->prev_x, term->prev_y,
                                          false);
        }
    }
    else
    {
        term->operations.draw_char_at(c, x, y, false);
    }
}

void terminal_add_newline(struct terminal *term)
{
    struct array new_line = ARRAY_INIT();
    array_add(&term->characters, &new_line, sizeof(struct array));
    /* term->operations.draw_char_at(' ', term->x, term->y, */
    /*                               false); /1* clear old cursor *1/ */
    term->y++;
    term->x = 0;

    terminal_draw_charat(terminal_getchar(*term, term->x, term->y), term->x,
                         term->y, term); /* add new cursor */

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
            struct array *a =
                array_get(&term->characters, term->y, sizeof(struct array));
            array_pop_middle(a, term->x - 1);
            term->x -= 1;

            char c = *(char *)array_get(a, term->x - 1, 1);
            terminal_draw_charat(c, term->x, term->y, term);
            for (size_t i = term->x + 1; i < a->len; i++)
            {
                char c2 = *(char *)array_get(a, i, 1);
                terminal_draw_charat(c2, i, term->y, term);
            }
            terminal_draw_charat(' ', a->len + 1, term->y, term);

            /* array_pop_end(line, 1); */
            /* term->operations.draw_char_at(' ', term->x + 1, term->y, false);
             */
        }
        break;
    default:;
        struct array *a =
            array_get(&term->characters, term->y, sizeof(struct array));
        char c2 = ' ';
        if (a->data != NULL)
        {
            c2 = *(char *)a->data;
        }

        array_insert(a, term->x, &c, 1);

        term->x++;
        /* terminal_set_pos(term, term->x + 1, term->y); */
        /* if (term->x < a->len - 1) */
        /* { */
        /*     term->operations.draw_char_at(((char *)a->data)[term->x],
         * term->x, */
        /*                                   term->y, false); */
        /* } */
        /* else */
        /* { */
        /*     term->operations.draw_char_at(' ', term->x, term->y, true); */
        /* } */
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
        /* term->operations.draw_char_at(' ', a->len + 1, term->y, true); */

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
    /* logd("%d", term->top_line); */
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
        /* FIXME: handle */

        return;
    }

    if (x <= 0)
    {
        x = 0;
    }
    else if (x >= line->len)
    {
        if (line->len <= 0)
        {
            x = 0;
        }
        else
        {
            x = line->len - 1;
        }
    }
    logd("%d, %d", x, y);

    /* terminal_draw_line(term, y, y); */
    char c = terminal_getchar(*term, term->x, term->y);
    terminal_draw_charat(c, term->x, term->y, term);

    terminal_set_pos(term, x, y);
    /* term->prev_x = term->x; */
    /* term->prev_y = term->y; */

    /* term->x = x; */
    /* term->y = y; */

    c = ((char *)line->data)[x];
    terminal_draw_charat(c, term->x, term->y, term);
}
