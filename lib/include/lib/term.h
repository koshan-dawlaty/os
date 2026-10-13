#ifndef LIB_TERM_H
#define LIB_TERM_H

#include <lib/array.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct terminal_operations
{
    void (*draw_char_at)(char c, size_t x, size_t y, bool is_cursor);
    size_t (*get_width)();
    size_t (*get_height)();
};
struct terminal
{
    struct terminal_operations operations;
    struct array characters; /* array of arrays */
    size_t top_line; /* line that is currently at the top of the screen */
    size_t x;
    size_t y;
    size_t num_written;

    size_t prev_x;
    size_t prev_y;
};

/*
 * adds a new line to @term
 *
 * intended for internal use by the terminal api
 */
void terminal_add_newline(struct terminal *term);

static inline struct terminal
terminal_init(struct terminal_operations operations)
{
    struct terminal out = {operations, ARRAY_INIT(), 0, 0, 0, 0, 0, 0};
    terminal_add_newline(&out);
    out.y = 0;

    return out;
}

static inline char terminal_getchar(struct terminal term, int x, int y)
{
    size_t num_lines = term.characters.len / sizeof(struct array);

    if (y <= 0)
    {
        y = 0;
    }
    else if (y >= (int)num_lines)
    {
        y = (int)num_lines - 1;
    }
    struct array *line = array_get(&term.characters, y, sizeof(struct array));

    if (line->len == 0)
    {
        return ' '; /* FIXME: error? */
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

    return *(char *)array_get(line, x, 1);
}

void terminal_add(const char *data, size_t len, struct terminal *term);
void terminal_addchar(char c, struct terminal *term);
void terminal_set_cursor(struct terminal *term, int x, int y);
/*
 * helper to increment based on current position
 */
static inline void terminal_move_cursor(struct terminal *term, int x, int y)
{
    terminal_set_cursor(term, term->x + x, term->y + y);
}

void terminal_draw_line(struct terminal *term, size_t index, size_t y);

void term_scroll_up(struct terminal *term);
void term_scroll_down(struct terminal *term);

#endif
