#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>
struct array
{
    size_t capacity;
    size_t len;
    void *data;
};

#define ARRAY_INIT() ((struct array){.capacity = 0, .len = 0, .data = NULL})
int array_add(struct array *array, const void *data, size_t size);

/*
 * you dont own the returned pointer, item_len is the length of one item in the
 * array, indexes as buffer + index * item_len
 */
void *array_get(struct array *array, size_t index, size_t item_len);
void *array_pop(struct array *array, size_t len);

#endif
