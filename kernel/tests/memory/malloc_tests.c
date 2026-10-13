// TODO: this file should be in libc
#include "kernel/paging.h"
#include "tests/tests.h"
#include <stdint.h>
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

    ASSERT_EQ((uintptr_t)one, (uintptr_t)two,
              "malloc should return the same pointer after an allocation and a "
              "free %p, %p",
              one, two);
})

test_result *malloc_test_multiple(size_t size, int amount)
{

    FAIL_TEST();
    char **ptrs = malloc(size * amount * sizeof(char));

    for (int i = 0; i < amount; i++)
    {
        char *ptr = malloc(size);
        *ptr = 'a';
        ptrs[i] = ptr;
    }

    for (int i = 0; i < amount; i++)
    {
        ASSERT_EQ(*(ptrs[i]), 'a');
    }

    for (int i = 0; i < amount; i++)
    {
        free(ptrs[i]);
    }

    free(ptrs);

    SUCCEED_TEST("");
}

TEST(malloc_test_diffrent_addrs, {
    int *one = malloc(10);
    int *two = malloc(10);

    ASSERT_NE((ptr_t)one, (ptr_t)two,
              "malloc returned the same pointer without for two allocations");

    free(one);
    free(two);
})

test_result *malloc_test_big_test()
{
    {
        int *one = malloc(4096);
        int *two = malloc(2048);
        int *three = malloc(2049);
        *one = 1;
        *two = 2;
        *three = 3;
        if (*one != 1)
        {
            return get_failed_test((int)0);
        };
        if (*two != 2)
        {
            return get_failed_test((int)0);
        };
        if (*three != 3)
        {
            return get_failed_test((int)0);
        };
        free(one);
        free(two);
        free(three);
    }
    return new_test_result(0, "");
}

TEST(malloc_test_big2, {
    int *one = malloc(4096);
    *one = 1;
    ASSERT_EQ(*one, 1);

    free(one);
    int *two = malloc(4096);

    int *three = malloc(2049);

    *two = 2;
    *three = 3;

    ASSERT_EQ(*two, 2);
    ASSERT_EQ(*three, 3);

    free(two);
    free(three);
})

TEST(malloc_test_multiple_small, { malloc_test_multiple(10, 10); })
TEST(malloc_test_multiple_large, { malloc_test_multiple(4096, 3); })
