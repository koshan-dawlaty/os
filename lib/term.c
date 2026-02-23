#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct terminal
{
    char **data;
    size_t data_len;

    size_t column_pos;
};

struct terminal *terminal_alloc()
{
    struct terminal *out = malloc(sizeof(struct terminal));

    out->data = malloc(sizeof(char *));
    out->data[0] = malloc(1);
    out->data[0][0] = 0;
    out->data_len = 1;

    out->column_pos = 0;

    return out;
}

void terminal_addchar(struct terminal *term, char c)
{
    switch (c)
    {
    case '\n':
        term->data_len++;
        term->data = realloc(term->data, term->data_len * sizeof(char *));
        break;

    default:;

        size_t len = strlen(term->data[term->data_len - 1]);
        term->data[term->data_len - 1] =
            realloc(term->data[term->data_len - 1], len + 1);
        term->data[term->data_len - 1][len] = c;
        term->data[term->data_len - 1][len + 1] = 0;

        break;
    }
}
