#ifndef TESTS_H
#define TESTS_H

#include "kernel/types.h"
#include <stdbool.h>
#include <stdint.h>

void run_all_tests();

typedef struct
{
    bool failed;
    char *message;

} test_result;

/*
 * the unused parameter has to be there so that varargs work, it has to be a
 * pointer because the stack has to be 16 byte aligned, and va_start gets an
 * argument from the stack
 */
test_result *get_failed_test(intmax_t, ...);

typedef test_result *(*callback)();

typedef struct
{
    callback func;
    char *name;
} test;
test_result *new_test_result(bool failed, char *message);

test *new_test(char *name, callback);
void add_test(test *);

// TODO: make these macros type independent
#define ASSERT_EQ(a, b, ...)                                                   \
    if (a != b)                                                                \
    {                                                                          \
        FAIL_TEST(__VA_ARGS__)                                                 \
    }

#define ASSERT_NE(a, b, ...)                                                   \
    if (a == b)                                                                \
    {                                                                          \
        FAIL_TEST(__VA_ARGS__);                                                \
    }

#define ASSERT_TRUE(a, ...)                                                    \
    if (!a)                                                                    \
    {                                                                          \
        FAIL_TEST(__VA_ARGS__);                                                \
    }

#define ASSERT_LT(a, b, ...)                                                   \
    if (a >= b)                                                                \
    {                                                                          \
        FAIL_TEST(__VA_ARGS__)                                                 \
    }

#define ASSERT_GT(a, b, ...)                                                   \
    if (a <= b)                                                                \
    {                                                                          \
        FAIL_TEST(__VA_ARGS__)                                                 \
    }

#define ASSERT_GE(a, b, ...)                                                   \
    if (a < b)                                                                 \
    {                                                                          \
        FAIL_TEST(__VA_ARGS__)                                                 \
    }

#define ASSERT_LE(a, b, ...)                                                   \
    if (a > b)                                                                 \
    {                                                                          \
        FAIL_TEST(__VA_ARGS__)                                                 \
    }

#define FAIL_TEST(...)                                                         \
    return get_failed_test(__VA_OPT__(1, )(int) 0 __VA_OPT__(, ) __VA_ARGS__);

#define SUCCEED_TEST(reason) return new_test_result(false, reason)

#ifdef TESTS
#define TEST_CONSTRUCTOR(name)                                                 \
                                                                               \
    __attribute__((constructor)) void name##_test_constructor()                \
    {                                                                          \
        add_test(new_test(#name, name##_test));                                \
    }

#else
#define TEST_CONSTRUCTOR(name)
#endif

#define TEST(name, body)                                                       \
    test_result *name##_test()                                                 \
    {                                                                          \
        body return new_test_result(false, "");                                \
    }                                                                          \
    TEST_CONSTRUCTOR(name)

#endif
