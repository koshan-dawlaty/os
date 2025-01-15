// TODO: this should be in libc
#include "kernel/paging.h"
#include "tests/tests.h"
#include <stdio.h>
#include <stdlib.h>

void malloc_tests_assert(int one, int two)
{
    if (one != two)
    {
        printf("malloc test failed\n%p != %p\n", (void *)one, (void *)two);
    }
}

void malloc_test_basic()
{
    test_debug_print("basic malloc test\n");

    int *one = malloc(12);
    *one = 12345;

    malloc_tests_assert(*one, 12345);
    free(one);
    int *two = malloc(12);

    *two = 67890;
    malloc_tests_assert(*two, 67890);
    free(two);

    malloc_tests_assert((int)one, (int)two);
}

void malloc_test_multiple(size_t size)
{
    test_debug_print("multiple malloc test, with size of: %d\n", size);
    int amount = 10;

    char **ptrs = malloc(size * amount * sizeof(char));

    for (int i = 0; i < amount; i++)
    {
        char *ptr = malloc(size);
        *ptr = 'a';
        ptrs[i] = ptr;
    }

    for (int i = 0; i < amount; i++)
    {
        malloc_tests_assert(*(ptrs[i]), 'a');
    }

    for (int i = 0; i < amount; i++)
    {
        free(ptrs[i]);
    }

    free(ptrs);
}

void malloc_test_diffrent_addrs()
{
    int *one = malloc(10);
    int *two = malloc(10);
    if (one == two)
    {
        printf("malloc test diffrent addrs failed!\nmalloc returned the same "
               "pointers!\n");
    }
    free(one);
    free(two);
}

void malloc_test_big()
{
    test_debug_print("malloc test big\n");

    int *one = malloc(4096);
    int *two = malloc(2048);
    int *three = malloc(2049);

    /* printf("1: %p, 2: %p, 3: %p\n", one, two, three); */

    *one = 1;
    *two = 2;
    *three = 3;

    malloc_tests_assert(*one, 1);
    malloc_tests_assert(*two, 2);
    malloc_tests_assert(*three, 3);

    free(one);
    free(two);
    free(three);
}

void malloc_test_big2()
{
    test_debug_print("malloc test big 2\n");

    int *one = malloc(4096);
    *one = 1;
    malloc_tests_assert(*one, 1);

    free(one);
    int *two = malloc(4096);

    // this does not work
    malloc_tests_assert((int)one, (int)two);
    int *three = malloc(2049);

    *two = 2;
    *three = 3;

    malloc_tests_assert(*two, 2);
    malloc_tests_assert(*three, 3);

    free(two);
    free(three);
}

void run_malloc_tests()
{
    malloc_test_basic();
    malloc_test_diffrent_addrs();
    malloc_test_multiple(10);

    malloc_test_big();
    malloc_test_big2();
    malloc_test_multiple(4096);
}
