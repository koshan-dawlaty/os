#ifndef ARRAY_H
#define ARRAY_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <types.h>

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

/*
 * macros that take a struct that looks like
 {
        type * items;
        size_t capacity;
        size_t num;
 }
 */

/* FIXME: these can maybe be improved by not making them macros, and instead
 * functions that take the size of the type */

#define ARRAY_DEFAULT_CAPACITY 2
#define array_macro_append(array, item)                                        \
    do                                                                         \
    {                                                                          \
        if ((array)->num >= (array)->capacity)                                 \
        {                                                                      \
            if ((array)->capacity == 0)                                        \
            {                                                                  \
                (array)->capacity = ARRAY_DEFAULT_CAPACITY;                    \
            }                                                                  \
            else                                                               \
            {                                                                  \
                (array)->capacity *= 2;                                        \
            }                                                                  \
            (array)->items = (typeof(*array.items))realloc(                    \
                (array)->items,                                                \
                (array)->capacity * sizeof(*((array)->items)));                \
        }                                                                      \
        (array)->items[(array)->num] = item;                                   \
        (array)->num++;                                                        \
    } while (0)

#define array_macro_append_buf(array, buf, n)                                  \
    do                                                                         \
    {                                                                          \
        if ((array)->num + (n) >= (array)->capacity)                           \
        {                                                                      \
            if ((array)->capacity == 0)                                        \
            {                                                                  \
                (array)->capacity = MAX(ARRAY_DEFAULT_CAPACITY, n);            \
            }                                                                  \
            else                                                               \
            {                                                                  \
                (array)->capacity = ((array)->num + n) * 2;                    \
            }                                                                  \
            (array)->items = realloc(                                          \
                (array)->items, (array)->capacity * sizeof(*(array)->items));  \
        }                                                                      \
        memcpy((array)->items + (array)->num, buf,                             \
               n * sizeof(*(array)->items));                                   \
        (array)->num += n;                                                     \
    } while (0)

#define array_macro_append_v(array, ...)                                       \
    do                                                                         \
    {                                                                          \
        typeof((array)->items) buf =                                           \
            (typeof (*(array)->items)[]){__VA_ARGS__};                         \
        array_macro_append_buf(                                                \
            array, buf,                                                        \
            sizeof((typeof (*(array)->items)[]){__VA_ARGS__}) /                \
                sizeof(*(array)->items));                                      \
    } while (0)

#define array_macro_remove_end(array)                                          \
    ((array)->num--, (array)->items[(array)->num])

#define array_at(array, index) ((array)->items[index])

#define array_copy_other(dest, source)                                         \
    (dest)->num = (source)->num;                                               \
    (dest)->capacity = (source)->capacity;                                     \
    (dest)->items = malloc((dest)->capacity * sizeof(*(dest)->items));         \
    memcpy((dest)->items, (source)->items, (dest)->num * sizeof(*(dest)->items))

#define array_remove(array, index)                                             \
    (array)->num--;                                                            \
    memmove((array)->items + index, (array)->items + (index - 1),              \
            sizeof(*(array)->items));

#endif
