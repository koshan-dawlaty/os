#ifndef TESTS_H
#define TESTS_H

#include <err_ptr.h>
#include <lib/utils.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <types.h>

void run_tests();

typedef struct test_result
{
    bool failed;
    char *message;

} test_result;

void test_output_str(const char *str, size_t len);

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

/* used for getting string literal / varargs from format output */
/* https://stackoverflow.com/questions/77894576/achieve-the-opposite-of-va-opt-in-variadic-preprocessor-function-like-macros*/
#define VALUE_IFNOT_1(...)
#define VALUE_IFNOT_0(...) __VA_ARGS__
#define VALUE_IFNOT_(...) __VA_ARGS__
#define VALUE_IFNOT(cond, ...) VALUE_IFNOT_##cond(__VA_ARGS__)
#define FIRST_ARG(first, ...) first
#define NOT_FIRST_ARG(first, ...) __VA_ARGS__
#define COMMA_IF_SECOND(first, ...) __VA_OPT__(, )

#define FAIL_TEST_EXTRA_REASON(a, why, ...)                                    \
    FAIL_TEST(why VALUE_IFNOT(__VA_OPT__(1), , ) FIRST_ARG(__VA_ARGS__)        \
                  __VA_OPT__(, ) MACRO_STR(a) COMMA_IF_SECOND(__VA_ARGS__)     \
                      __VA_OPT__(NOT_FIRST_ARG(__VA_ARGS__)))

#define FAIL_TEST_EXTRA_REASON2(a, b, why, ...)                                \
    FAIL_TEST(why VALUE_IFNOT(__VA_OPT__(1), , ) FIRST_ARG(__VA_ARGS__)        \
                  __VA_OPT__(, ) MACRO_STR(a),                                 \
              MACRO_STR(b) COMMA_IF_SECOND(__VA_ARGS__)                        \
                  __VA_OPT__(NOT_FIRST_ARG(__VA_ARGS__)))

// TODO: make these macros type independent
#define ASSERT_EQ(a, b, ...)                                                   \
    if (a != b)                                                                \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON2(a, b, "%s != %s ", __VA_ARGS__)                \
    }

#define ASSERT_NE(a, b, ...)                                                   \
    if (a == b)                                                                \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON2(a, b, "%s == %s ", __VA_ARGS__)                \
    }

#define ASSERT_TRUE(a, ...)                                                    \
    if (!a)                                                                    \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON(a, "%s was true ", __VA_ARGS__)                 \
    }

#define ASSERT_LT(a, b, ...)                                                   \
    if (a >= b)                                                                \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON2(a, b, "%s >= %s ", __VA_ARGS__)                \
    }

#define ASSERT_GT(a, b, ...)                                                   \
    if (a <= b)                                                                \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON2(a, b, "%s <= %s ", __VA_ARGS__)                \
    }

#define ASSERT_GE(a, b, ...)                                                   \
    if (a < b)                                                                 \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON2(a, b, "%s < %s ", __VA_ARGS__)                 \
    }

#define ASSERT_LE(a, b, ...)                                                   \
    if (a > b)                                                                 \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON2(a, b, "%s > %s ", __VA_ARGS__)                 \
    }
#define ASSERT_EQ_STR(a, b, ...)                                               \
    if (strcmp(a, b) != 0)                                                     \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON2(a, b, "%s != %s ", __VA_ARGS__)                \
    }
#define ASSERT_NONULL(ptr, ...)                                                \
    if (ptr == NULL)                                                           \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON(ptr, "%s was null", __VA_ARGS__)                \
    }
#define ASSERT_NOERR(value, ...)                                               \
    if (IS_ERR(value))                                                         \
    {                                                                          \
        FAIL_TEST_EXTRA_REASON(value, "%s had error", __VA_ARGS__)             \
    }

#define FAIL_TEST(...)                                                         \
    return get_failed_test(__VA_OPT__(1, )(int) 0 __VA_OPT__(, ) __VA_ARGS__);

#define SUCCEED_TEST(reason) return new_test_result(false, reason)

/*
 * useful for functions that return a struct test_result * to have a default
 * value
 */
#define TEST_OK() return new_test_result(false, "");

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

#define TEST(name, body, ...)                                                  \
    test_result *name##_test(){                                                \
                                                                               \
        body TEST_OK()                                                         \
                                                                               \
    } TEST_CONSTRUCTOR(name)

#endif
