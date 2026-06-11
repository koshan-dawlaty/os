#ifndef ARRAY_H
#define ARRAY_H

#include "kernel/types.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct array
{
    size_t capacity;
    size_t len;
    void *data;
};

#define ARRAY_LEN(array, size) (((array).len) / size)

#define ARRAY_INIT() ((struct array){.capacity = 0, .len = 0, .data = NULL})
int array_add(struct array *array, const void *data, size_t size);

/*
 * you dont own the returned pointer
 *
 * item_len is the length of one item in the array, indexes as buffer + index *
 * item_len
 */
void *array_get(struct array *array, size_t index, size_t item_len);

/* this removes from the end of the array, therefore this does not
 * invalidate indexes, and it returns a pointer inside the array */
void *array_pop_end(struct array *array, size_t len);
/*
 * this removes from the start of the array and invalidates indexes
 * returns a heap pointer
 */
void *array_pop(struct array *array, size_t len);

/* invalidates indexes of everything else in the array */
void array_push(struct array *array, const void *data, size_t size);

/*
 * does not assume @array is a heap pointer
 */
static inline void array_free(struct array *array)
{
    free(array->data);
}

/*
 * frees the array, then, returns a heap pointer to the array data
 *
 * as of right now this just returns the data, this would be useful if the array
 * had data that needed to be cleaned up
 *
 * this effectively makes the array immutable
 */
static inline void *array_free_ptr(struct array array)
{
    return array.data;
}

static inline void array_set(struct array *array, size_t index, void *data,
                             size_t len)
{
    size_t index_real = (index)*len;
    if (array->len < index_real)
    {
        printf("index %d out of bounds for len %d\n", index_real, array->len);
    }
    printf("writing to:%d for index %d\n", index_real, index);
    memcpy(array->data + index_real, data, len);
}

/*
 * insert @data into @array at @offset
 * @offset is in bytes
 */
void array_insert(struct array *array, size_t offset, void *data, size_t len);

u8 array_pop_middle(struct array *array, size_t offset);

#endif
