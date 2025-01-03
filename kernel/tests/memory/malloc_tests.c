#include "kernel/paging.h"
#include "tests/tests.h"
#include <stdio.h>

void malloc_test_basic()
{
    test_debug_print("basic malloc test\n");

    int *one = kmalloc(12);
    int *two = kmalloc(12);
    *one = 12345;
    if (*one != 12345)
    {
        printf("basic malloc test failed\none had an invalid value\n");
        return;
    }
    kfree(one);

    *two = 67890;
    if (*two != 67890)
    {

        printf("basic malloc test failed\ntwo had an invalid value\n");
        return;
    }
    kfree(two);

    if (one != two)
    {
        printf("basic malloc test failed!\npointers were not equal\n");
        return;
    }
}

void run_malloc_tests()
{
    malloc_test_basic();
}
