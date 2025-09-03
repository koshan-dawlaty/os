#ifndef TESTS_H
#define TESTS_H

void run_all_tests();

typedef void (*callback)();

typedef struct
{
    callback func;
    char *name;
} Test;

Test *new_test(char *name, callback);
void add_test(Test *);

void fail_test();

void test_assert_eq(int a, int b);
void test_assert_ne(int a, int b);
void test_assert_true(int condition);

// TODO: make these macros type independent
#define ASSERT_EQ(a, b) test_assert_eq(a, b)
#define ASSERT_NE(a, b) test_assert_ne(a, b)
#define ASSERT_TRUE(a) test_assert_true(a)
#define FAIL_TEST() fail_test();

#ifdef TESTS
#define TEST_CONSTRUCTOR(name)                                                 \
                                                                               \
    __attribute__((constructor)) void name##_test_constructor()                \
    {                                                                          \
        add_test(new_test(#name, name##_test));                                \
    }

#else
#define TEST_CONSTRUCTOR(name)
#endif

#define TEST(name, body) void name##_test() body TEST_CONSTRUCTOR(name)

#endif
