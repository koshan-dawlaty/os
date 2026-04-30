#ifndef ASSERT_H
#define ASSERT_H

#include <kernel/utils.h>
#include <lib/utils.h>

void __panic_func(const char *format, ...);

#define __panic_impl(format, ...)                                              \
    __panic_func(__FILE__ ":" MACRO_STR(__LINE__) " " format __VA_OPT__(, )    \
                     __VA_ARGS__)

/* assert that something is true, if it is not, panic */
#define ASSERT(condition, message, ...)                                        \
    if (!(condition))                                                          \
    {                                                                          \
        __panic_impl("assertion failed\n" message __VA_OPT__(, ) __VA_ARGS__); \
    }

#define PANIC(message, ...) __panic_impl(message __VA_OPT__(, ) __VA_ARGS__)
/* preferred to use PANIC */
#define panic(message, ...) __panic_impl(message __VA_OPT__(, ) __VA_ARGS__)

/* panics if pointer is null */
#define NOT_NULL(pointer, message, ...)                                        \
    ASSERT(pointer != NULL, "pointer was null\n" message, __VA_ARGS__)

#define UNREACHABLE(message, ...)                                              \
    __panic_impl("unreachable code\n" message __VA_OPT__(, ) __VA_ARGS__);     \
    __builtin_unreachable()

#endif
