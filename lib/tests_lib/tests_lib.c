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

#ifdef __is_libk
#include <serial.h>
#endif

#define TEST_OUTPUT_SERIAL_PORT 0x2F8

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

struct test_result *test_run(struct test *test)
{
    return test->func();
}

/* returns true if @testname matches @pattern */
bool test_pattern_matches(const char *pattern, const char *testname)
{
    size_t i = 0;
    while (pattern[i] != 0 && testname[i] != 0 &&
           (testname[i] == pattern[i] || pattern[i] == '*'))
    {
        if (pattern[i] == '*')
        {
            /* at this point, every character has been in the pattern, so seeing
             * a star here, meaning match until end, would mean that it matched
             */
            return true;
        }
        i++;
    }
    return pattern[i] == testname[i];
}

extern struct test __start_test_vars;
extern struct test __stop_test_vars;

#define TESTS_LEN (&__stop_test_vars - &__start_test_vars)
#define TESTS_ARRAY (&__start_test_vars)

void tests_run_input(struct test_input *input)
{

    char *tap_header = tap_get_header(TESTS_LEN);
    test_output_str(tap_header, strlen(tap_header));
    free(tap_header);

    for (size_t i = 0; i < TESTS_LEN; i++)
    {
        struct test current_test = TESTS_ARRAY[i];
        for (size_t j = 0; j < input->test_patterns.num; j++)
        {

            bool matched = test_pattern_matches(input->test_patterns.items[j],
                                                current_test.name);
            if ((matched && input->test_input_type == TEST_INPUT_ONLY) ||
                (!matched && input->test_input_type == TEST_INPUT_EXCLUDE))
            {
                test_result *result = test_run(&current_test);

                char *tap = tap_format_test_result(&current_test, result, i);
                test_output_str(tap, strlen(tap));

                free(result);

                free(tap);
            }
        }
    }

    /* end of input */
    test_output('\n');
    test_output(4);
}
