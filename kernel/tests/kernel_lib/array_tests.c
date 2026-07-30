#include <lib/array.h>
#include <tests/tests.h>

TEST(array_basic, {
    struct array a = ARRAY_INIT();
    const char *str1 = "abc";
    const char *str2 = "123";
    const char *str3 = "456";
    array_add(&a, str1, 4);
    array_add(&a, str2, 4);
    array_add(&a, str3, 4);

    const char *result1 = array_get(&a, 0, 4);
    const char *result2 = array_get(&a, 1, 4);
    const char *result3 = array_get(&a, 2, 4);
    ASSERT_EQ_STR(result1, str1, "%s != %s", result1, str1);
    ASSERT_EQ_STR(result2, str2, "%s != %s", result2, str2);
    ASSERT_EQ_STR(result3, str3, "%s != %s", result3, str3);
})

struct test_array
{
    int *items;
    size_t num;
    size_t capacity;
};

TEST(array_macro_basic, {
    struct test_array array = {0};
    for (size_t i = 0; i < 10; i++)
    {
        array_macro_append(&array, i);
    }
    ASSERT_EQ(array.num, 10, "%d != %d", array.num, 10);
    for (size_t i = 0; i < array.num; i++)
    {
        ASSERT_EQ(array.items[i], (int)i);
    }

    for (int i = 9; i >= 0; i--)
    {
        int num = array_macro_remove_end(&array);
        ASSERT_EQ(num, i);
    }
    ASSERT_EQ(array.num, 0);
})

TEST(array_macro_append_v, {
    struct test_array array = {0};
    array_macro_append_v(&array, 123, 456, 789, 0);
    ASSERT_EQ(array.items[0], 123);
    ASSERT_EQ(array.items[1], 456);
    ASSERT_EQ(array.items[2], 789);
    ASSERT_EQ(array.items[3], 0);
})
