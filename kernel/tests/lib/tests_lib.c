#include "serial.h"
#include "tap.h"
#include "tests/tests.h"
#include <extension.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEST_OUTPUT_SERIAL_PORT 0x3F8

test **tests = NULL;
size_t tests_size = 0;

static int failed = 0;

test_result *new_test_result(bool failed, char *message)
{
    test_result *out = malloc(sizeof(test_result));
    out->failed = failed;
    out->message = message;

    return out;
}
test_result *get_failed_test(intmax_t num, ...)
{
    va_list args;
    va_start(args, num);

    char *reason;

    const char *format = NULL;
    if (num == 1)
    {
        /* due to how the macro works, there will always be a zero in the
         * varargs */
        va_arg(args, int);
        format = va_arg(args, const char *);

        reason = vrprintf(format, args);
    }

    va_end(args);

    if (num == 0)
    {
        reason = NULL;
    }
    test_result *out = new_test_result(true, reason);

    return out;
}

// TODO: could improve this and test_assert_eq later
void fail_test()
{
    printf("test failed\n");
    failed = 1;
}

void test_assert_eq(int a, int b)
{
    if (a != b)
    {
        fail_test();
        printf("%d != %d\n", a, b);
    }
}

void test_assert_ne(int a, int b)
{
    if (a == b)
    {
        fail_test();
        printf("%d == %d\n", a, b);
    }
}

void test_assert_true(int condition)
{
    if (!condition)
    {
        fail_test();
        printf("condition was false\n");
    }
}

test *new_test(char *name, callback func)
{
    /* printf("calling malloc!!\n"); */
    test *out = malloc(sizeof(test));
    /* printf("pointer is:%p, size is:%p, name is:%s\n", out, out + 1, name); */
    /* out->name = strdup("abcdefg"); */
    out->name = strdup(name);
    out->func = func;

    return out;
}

void add_test(test *t)
{
    tests_size++;
    tests = realloc(tests, tests_size * sizeof(test));
    tests[tests_size - 1] = t;
}

/*
 * returns true if the test failed
 */
test_result *run_test(test *test)
{
    test_result *result = test->func();

    return result;
}

void run_all_tests()
{
    /* printf("running tests\n"); */

    int *failed_indexes = malloc(sizeof(size_t) * tests_size);
    test_result **results = malloc(sizeof(test_result) * tests_size);
    /* printf("this is:%p\n", failed_indexes); */
    size_t failed_indexes_len = 0;

    for (size_t i = 0; i < tests_size; i++)
    {
        /* if ((uint32_t)tests[i] < 100) */
        /* { */
        /*     printf("addr: %p\n", tests[i]); */
        /* } */

        /* printf("running: %s\n", tests[i]->name); */
        test_result *result = run_test(tests[i]);

        if (result->failed)
        {
            failed_indexes[failed_indexes_len] = i;
            failed_indexes_len++;
        }

        results[i] = result;
    }

    /* serial_write(TEST_OUTPUT_SERIAL_PORT, failed_indexes_len + 0x30); */
    /* for (size_t i = 0; i < failed_indexes_len; i++) */
    /* { */
    /*     test *t = tests[failed_indexes[i]]; */
    /*     /1* test_result *r = results[i]; *1/ */
    /*     printf("test %s failed!\n", t->name); */
    /*     /1* if (r->message != NULL) *1/ */
    /*     /1* { *1/ */
    /*     /1*     printf("reason:%s\n", r->message); *1/ */
    /*     /1* } *1/ */
    /* } */

    free(failed_indexes);

    char *tap = format_test_results_tap(tests, results, tests_size);

    for (size_t i = 0; i < tests_size; i++)
    {
        free(results[i]);
    }

    serial_writestring(tap, strlen(tap), TEST_OUTPUT_SERIAL_PORT);

    /* printf("%s\n", tap); */

    /* end of input */
    serial_write(TEST_OUTPUT_SERIAL_PORT, 4);
}
