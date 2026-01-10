#include "assert.h"
#include <lib/array.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

int array_add(struct array *array, const void *data, size_t size)
{
    if (array->len + size > array->capacity)
    {
        array->capacity += array->len + size - array->capacity;
        array->data = realloc(array->data, array->capacity);
    }
    memcpy(array->data + array->len, data, size);
    array->len += size;

    return (array->len - size) / size;
}

void *array_get(struct array *array, size_t index, size_t item_len)
{
    /* TODO: could implement this function as a macro so that this prints the
     * line number where this was called */
    ASSERT(index * item_len <= array->len,
           "index %d out of bounds for array len %d", index * item_len,
           array->len);
    return array->data + index * item_len;
}

void *array_pop(struct array *array, size_t len)
{
    ASSERT(array->len >= len, "array length too high, %d > %d", len,
           array->len);

    void *out = array->data + array->capacity - len;

    array->len -= len;

    return out;
}
