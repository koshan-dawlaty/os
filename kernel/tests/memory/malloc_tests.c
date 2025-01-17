// TODO: this file should be in libc
#include "kernel/paging.h"
#include "tests/tests.h"
#include <stdio.h>
#include <stdlib.h>

TEST(malloc_test_basic, {
    /* printf("test\n"); */
    int *one = malloc(12);
    *one = 12345;

    ASSERT_EQ(*one, 12345);
    free(one);
    int *two = malloc(12);

    *two = 67890;
    ASSERT_EQ(*two, 67890);
    free(two);

    ASSERT_EQ((int)one, (int)two);
})

void malloc_tests_assert(int one, int two)
{
    if (one != two)
    {
        printf("malloc test failed\n%p != %p\n", (void *)one, (void *)two);
    }
}

void malloc_test_multiple(size_t size)
{
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
    malloc_test_diffrent_addrs();
    malloc_test_multiple(10);

    malloc_test_big();
    malloc_test_big2();
    malloc_test_multiple(4096);
}
