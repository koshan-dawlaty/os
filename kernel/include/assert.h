#ifndef ASSERT_H
#define ASSERT_H

#include <panic.h>

/* assert that something is true, if it is not, panic */
#define ASSERT(condition, message, ...)                                        \
    if (!(condition))                                                          \
    {                                                                          \
        panic(message __VA_OPT__(, ) __VA_ARGS__);                             \
    }

/* panics if pointer is null */
#define NOT_NULL(pointer, message, ...)                                        \
    ASSERT(pointer != NULL, message __VA_OPT__(, ) __VA_ARGS__)

#endif
