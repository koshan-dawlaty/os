#include "assert.h"
#include <container_of.h>
#include <lib/lru.h>
#include <stdio.h>
#include <stdlib.h>
#include <tests/tests.h>

struct lru_test
{
    int data;
    struct lru_entry lru;
};

struct lru_test *new_lru_test(int d)
{
    struct lru_test *out = malloc(sizeof(struct lru_test));

    out->data = d;
    out->lru = LRU_ENTRY_INIT();

    return out;
}

bool compare_test_lru(struct lru_entry *entry, void *parameter)
{
    struct lru_test *test = containerof(entry, struct lru_test, lru);
    /* printf("%d, %d\n", test->data, (int)(ptr)parameter); */
    return test->data == (int)(ptr)parameter;
}

TEST(lru_test_basic, {
    struct lru_test *one = new_lru_test(1);
    struct lru_test *two = new_lru_test(2);
    struct lru_test *three = new_lru_test(3);

    struct lru_head *head = new_lru_head(2);

    lru_add(head, &one->lru);
    lru_add(head, &two->lru);
    lru_add(head, &three->lru);

    void *not_exist = lru_get(head, compare_test_lru, (void *)1);
    ASSERT_EQ(not_exist, NULL, "value not in list should be null, was %p",
              not_exist);

    struct lru_test *value2 = containerof(
        lru_get(head, compare_test_lru, (void *)2), struct lru_test, lru);

    struct lru_test *value3 = containerof(
        lru_get(head, compare_test_lru, (void *)3), struct lru_test, lru);

    /* ASSERT_EQ(value1, NULL, "value not in list should be null, was %p",
     */
    /*           value1) */
    ASSERT_EQ(value2->data, 2)
    ASSERT_EQ(value3->data, 3)

    ASSERT_EQ(lru_get(head, compare_test_lru, (void *)0), NULL,
              "non existant value should return null");
})
