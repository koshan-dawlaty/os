#ifndef TERMINAL_LIB_H
#define TERMINAL_LIB_H

#include <stddef.h>

struct terminal
{
    char **data;
    size_t data_len;

    size_t column_pos;
};

struct terminal *terminal_alloc();
void terminal_addchar(struct terminal *term, char c);

#endif
