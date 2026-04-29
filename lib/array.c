#include "lib/log.h"
#include <lib/array.h>
#include <lib/assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

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

/* invalidates indexes of everything else in the array */
void array_push(struct array *array, const void *data, size_t size)
{
    if (array->len + size > array->capacity)
    {
        array->capacity += array->len + size - array->capacity;
        array->data = realloc(array->data, array->capacity);
    }

    u8 *dest = array->data + size;
    u8 *src = array->data;
    if (array->len != 0)
    {
        /* overlapping copy */
        for (size_t i = array->len; i > 0; i--)
        {
            dest[i - 1] = src[i - 1];
        }
    }

    memcpy(array->data, data, size);
    array->len += size;
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

void *array_pop_end(struct array *array, size_t len)
{
    ASSERT(array->len >= len, "array length too high, %d > %d", len,
           array->len);

    void *out = array->data + array->len - len;

    array->len -= len;

    return out;
}

void *array_pop(struct array *array, size_t len)
{
    if (array->len < len)
    {
        printf("array length too high %d > %d\n", len, array->len);
        while (1)
        {
        }
    }

    void *out = malloc(len);
    memcpy(out, array->data, len);

    /* we cant just increment the pointer because it will be passed to realloc()
     * later */
    /* array->data += len; */
    /* TODO: do a overlapping copy */
    void *temp = malloc(array->len);
    memcpy(temp, array->data, array->len);
    memcpy(array->data, temp + len, array->len - len);

    /* printf("read:%s, %d, %s, %s\n", array->data, array->len, temp, out); */
    /* printf("read:%s, %d\n", array->data, len); */
    array->len -= len;

    return out;
}

void array_insert(struct array *array, size_t offset, void *data, size_t size)
{

    if (array->len + size > array->capacity)
    {
        array->capacity += array->len + size - array->capacity;
        array->data = realloc(array->data, array->capacity);
    }

    u8 *dest = array->data + offset + size;
    u8 *src = array->data + offset;

    if (array->len != 0)
    {
        for (size_t i = array->len; i > 0; i--)
        {
            dest[i - 1] = src[i - 1];
        }

        /* for (size_t i = 0; i < array->len; i++) */
        /* { */
        /*     dest[i - 1] = src[i - 1]; */
        /* } */
    }

    memcpy(array->data + offset, data, size);
    array->len += size;
}

u8 array_pop_middle(struct array *array, size_t offset)
{
    ASSERT(offset < array->len, "array index %d out of bounds for len %d",
           offset, array->len);

    u8 retval = ((u8 *)array->data)[offset];

    u8 *dest = array->data + offset;
    u8 *src = array->data + offset + 1;

    if (array->len != 0)
    {
        /* for (size_t i = array->len; i > 0; i--) */
        /* { */
        /*     dest[i - 1] = src[i - 1]; */
        /* } */

        for (size_t i = 0; i < array->len; i++)
        {
            dest[i - 1] = src[i - 1];
        }
    }

    array->len--;
    return retval;
}
