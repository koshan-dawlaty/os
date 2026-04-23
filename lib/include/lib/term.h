#ifndef LIB_TERM_H
#define LIB_TERM_H

#include <lib/array.h>
#include <stddef.h>
#include <stdint.h>

struct terminal_operations
{
    void (*draw_char_at)(char c, size_t x, size_t y);
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
    struct terminal out = {operations, ARRAY_INIT(), 0, 0, 0};
    terminal_add_newline(&out);
    return out;
}

void terminal_add(const char *data, size_t len, struct terminal *term);
void terminal_addchar(char c, struct terminal *term);

#endif
