#include "assert.h"
#include "lib/list.h"
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

    ASSERT_EQ(value2->data, 2)
    ASSERT_EQ(value3->data, 3)

    ASSERT_EQ(lru_get(head, compare_test_lru, (void *)0), NULL,
              "non existant value should return null");
})

TEST(lru_test2, {
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

    ASSERT_EQ(value2->data, 2)
    ASSERT_EQ(value3->data, 3)

    ASSERT_EQ(lru_get(head, compare_test_lru, (void *)0), NULL,
              "non existant value should return null");

    struct lru_test *four = new_lru_test(4);
    struct lru_test *five = new_lru_test(5);
    struct lru_test *six = new_lru_test(6);

    lru_add(head, &four->lru);
    lru_add(head, &five->lru);
    struct lru_test *value5 = containerof(
        lru_get(head, compare_test_lru, (void *)5), struct lru_test, lru);
    ASSERT_EQ(value5->data, 5)

    lru_add(head, &six->lru);

    struct lru_test *value6 = containerof(
        lru_get(head, compare_test_lru, (void *)6), struct lru_test, lru);
    ASSERT_EQ(value6->data, 6)

    struct list2 *pos;
    size_t i = 0;
    list2_for_each(pos, &head->head->list)
    {
        struct lru_test *t = LRU_LIST_ENTRY(pos, struct lru_test, lru);

        switch (i)
        {
        case 0:
            ASSERT_EQ(t->data, 6, "most recent entry should be at head")
            break;
        case 1:
            ASSERT_EQ(t->data, 5, "least recent entry should be at tail")
            break;
        default:
            FAIL_TEST("list is too long")
            break;
        }

        i++;
    }
})
