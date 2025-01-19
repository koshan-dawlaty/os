#include "tests/tests.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Test **tests = NULL;
size_t tests_size = 0;

static int failed = 0;

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

Test *new_test(char *name, callback func)
{
    /* printf("calling malloc!!\n"); */
    Test *out = malloc(sizeof(Test));
    /* printf("pointer is:%p, size is:%p, name is:%s\n", out, out + 1, name); */
    /* out->name = strdup("abcdefg"); */
    out->name = strdup(name);
    out->func = func;

    return out;
}

void add_test(Test *test)
{
    /* printf("tests points to:%p\n", tests); */
    /* printf("%d\n", tests_size); */
    /* printf("add_test called for %s\n", test->name); */
    tests_size++;
    tests = realloc(tests, tests_size * sizeof(Test));

    tests[tests_size - 1] = test;
}

/*
 * returns true if the test failed
 */
int run_test(Test *test)
{
    failed = 0;
    test->func();

    return failed;
}

void run_all_tests()
{
    /* printf("running tests\n"); */
    int *failed_indexes = malloc(sizeof(int) * tests_size);
    /* printf("this is:%p\n", failed_indexes); */
    size_t failed_indexes_index = 0;

    for (size_t i = 0; i < tests_size; i++)
    {
        /* if ((uint32_t)tests[i] < 100) */
        /* { */
        /*     printf("addr: %p\n", tests[i]); */
        /* } */

        printf("running: %s\n", tests[i]->name);
        if (run_test(tests[i]))
        {
            failed_indexes[failed_indexes_index] = i;
            failed_indexes_index++;
        }
    }

    for (size_t i = 0; i < failed_indexes_index; i++)
    {
        /* printf("test %s failed!\n", tests[i]->name); */
    }
}
