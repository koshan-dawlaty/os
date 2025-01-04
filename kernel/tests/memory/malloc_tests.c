#include "kernel/paging.h"
#include "tests/tests.h"
#include <stdio.h>

void malloc_tests_assert(int one, int two)
{

    if (one != two)
    {
        printf("malloc test failed\n%p != %p", (void *)one, (void *)two);
    }
}

void malloc_test_basic()
{
    test_debug_print("basic malloc test\n");

    int *one = kmalloc(12);
    *one = 12345;

    malloc_tests_assert(*one, 12345);
    kfree(one);
    int *two = kmalloc(12);

    *two = 67890;
    malloc_tests_assert(*two, 67890);
    kfree(two);

    malloc_tests_assert((int)one, (int)two);
}

void malloc_test_multiple()
{
    test_debug_print("multiple malloc test\n");
    int size = 10;
    int amount = 10;

    char **ptrs = kmalloc(size * amount * sizeof(char));

    for (int i = 0; i < amount; i++)
    {
        char *ptr = kmalloc(size);
        *ptr = 'a';
        ptrs[i] = ptr;
    }

    for (int i = 0; i < amount; i++)
    {
        malloc_tests_assert(*(ptrs[i]), 'a');
    }

    for (int i = 0; i < amount; i++)
    {
        kfree(ptrs[i]);
    }

    kfree(ptrs);
}

void run_malloc_tests()
{
    malloc_test_basic();
    malloc_test_multiple();
}
