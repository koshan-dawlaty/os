#include "kernel/paging.h"
#include "lib/log.h"
#include "tests/tests.h"
#include <memory/get_free_pages.h>
#include <stdio.h>
#include <stdlib.h>

/* #error "these tests seem to cause a page fault" */

TEST(get_free_pages_test_basic, {
    int *one = get_free_pages(1);
    int *two = get_free_pages(1);
    int *three = get_free_pages(1);

    /* printf("1: %p, 2: %p, 3: %p\n", one, two, three); */

    *one = 1;
    *two = 2;
    *three = 3;

    ASSERT_EQ(*one, 1);
    ASSERT_EQ(*two, 2);
    ASSERT_EQ(*three, 3);

    free_pages(one, 1);
    free_pages(two, 1);
    free_pages(three, 1);
})

TEST(get_free_pages_test_multiple, {
    int amount = 10;
    int size = 1;
    int **ptrs = malloc(amount * sizeof(void *));
    for (int i = 0; i < amount; i++)
    {
        ptrs[i] = get_free_pages(size);
        *(ptrs[i]) = i;
    }
    for (int i = 0; i < amount; i++)
    {
        if (*ptrs[i] != i)
        {
            return get_failed_test((int)0);
        };
    }
    for (int i = 0; i < amount; i++)
    {
        free_pages(ptrs[i], size);
    }
    return new_test_result(0, "");
})
