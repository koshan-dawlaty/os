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
