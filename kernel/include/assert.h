#ifndef ASSERT_H
#define ASSERT_H

#include <kernel/utils.h>
#include <panic.h>

/* assert that something is true, if it is not, panic */
#define ASSERT(condition, message, ...)                                        \
    if (!(condition))                                                          \
    {                                                                          \
        panic("assertion failed\n" __FILE__                                    \
              ":" MACRO_STR(__LINE__) " " message __VA_OPT__(, ) __VA_ARGS__); \
    }

/* panics if pointer is null */
#define NOT_NULL(pointer, message, ...)                                        \
    ASSERT(pointer != NULL,                                                    \
           "pointer was null\n" message __VA_OPT__(, ) __VA_ARGS__)

#endif
