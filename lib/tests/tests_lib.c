#ifdef __is_libk
#include "serial.h"
#endif
#include "tap.h"
#include <extension.h>
#include <fcntl.h>
#include <lib/assert.h>
#include <lib/log.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tests/tests.h>
#include <unistd.h>

#define TEST_OUTPUT_SERIAL_PORT 0x3F8

#ifdef TESTS_EXTRA_TESTS
/* defined in test.sh */
const char *extra_tests[] = {TESTS_EXTRA_TESTS};
#else
const char *extra_tests[] = {};
#endif

test **tests = NULL;
size_t tests_size = 0;

static int failed = 0;

int serialfd = 3;

void test_output(char c)
{
#ifdef __is_libk
    serial_write(TEST_OUTPUT_SERIAL_PORT, c);
#else
    write(serialfd, &c, 1);

#endif
}

void test_output_str(const char *str, size_t len)
{
#ifdef __is_libk
    serial_writestring(str, len, TEST_OUTPUT_SERIAL_PORT);
#else
    write(serialfd, str, len);

#endif
}

void test_init_output()
{
#ifdef __is_libk
    serial_init(TEST_OUTPUT_SERIAL_PORT);

#endif
}

test_result *new_test_result(bool failed, char *message)
{
    test_result *out = malloc(sizeof(test_result));
    out->failed = failed;
    if (message != NULL)
    {
        out->message = strdup(message);
    }
    else
    {
        out->message = NULL;
    }

    return out;
}
test_result *get_failed_test(intmax_t num, ...)
{
    va_list args;
    va_start(args, num);

    char *reason = "";

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

    size_t failed_indexes_len = 0;

    char *tap_header = tap_get_header(tests_size);
    test_output_str(tap_header, strlen(tap_header));
    free(tap_header);

    for (size_t i = 0; i < tests_size; i++)
    {

        printf("running: %s\n", tests[i]->name);
        test_result *result = run_test(tests[i]);

        if (result->failed)
        {
            failed_indexes[failed_indexes_len] = i;
            failed_indexes_len++;
        }

        char *tap = tap_format_test_result(tests[i], result, i);
        test_output_str(tap, strlen(tap));

        free(tap);

        results[i] = result;
    }

    free(failed_indexes);

    for (size_t i = 0; i < tests_size; i++)
    {
        free(results[i]);
    }

    /* end of input */
    test_output('\n');
    test_output(4);
}

/* runs all of the tests matching names in extra_tests */
void run_all_tests_only()
{
    const size_t num_extra = sizeof(extra_tests) / sizeof(const char *);

    char *tap_header = tap_get_header(num_extra);
    test_output_str(tap_header, strlen(tap_header));
    free(tap_header);

    for (size_t i = 0; i < num_extra; i++)
    {
        const char *name = extra_tests[i];
        for (size_t j = 0; j < tests_size; j++)
        {
            test *t = tests[j];

            if (!strcmp(t->name, name))
            {
                printf("running:%s\n", t->name);
                test_result *r = run_test(t);
                char *tap = tap_format_test_result(tests[j], r, i);
                test_output_str(tap, strlen(tap));

                free(tap);
            }
        }
    }

    test_output('\n');
    test_output(4);
}

void run_tests()
{
    test_init_output();
#ifdef TESTS_ONLY
    run_all_tests_only();
#else
    run_all_tests();
#endif
}
