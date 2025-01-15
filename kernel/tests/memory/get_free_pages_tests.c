#include "kernel/paging.h"
#include "tests/tests.h"
#include <stdio.h>

void paging_tests_assert(int one, int two)
{
    if (one != two)
    {
        printf("paging test failed\n%p != %p\n", (void *)one, (void *)two);
    }
}

void get_free_pages_test_basic()
{
    test_debug_print("basic get free pages test\n");

    int *one = get_free_pages(1);
    int *two = get_free_pages(1);
    int *three = get_free_pages(1);

    /* printf("1: %p, 2: %p, 3: %p\n", one, two, three); */

    *one = 1;
    *two = 2;
    *three = 3;

    paging_tests_assert(*one, 1);
    paging_tests_assert(*two, 2);
    paging_tests_assert(*three, 3);

    free_pages(one, 1);
    free_pages(two, 1);
    free_pages(three, 1);
}

void get_free_pages_test_multiple()
{
    test_debug_print("multiple get free pages test\n");

    int amount = 10;
    int size = 1;
    // bad
    int **ptrs = kmalloc(amount * sizeof(void *));

    for (int i = 0; i < amount; i++)
    {
        ptrs[i] = get_free_pages(size);
        *(ptrs[i]) = i;
    }

    for (int i = 0; i < amount; i++)
    {
        paging_tests_assert(*ptrs[i], i);
    }

    for (int i = 0; i < amount; i++)
    {
        free_pages(ptrs[i], size);
    }
}

void run_get_free_pages_tests()
{
    get_free_pages_test_basic();
    get_free_pages_test_multiple();
}
