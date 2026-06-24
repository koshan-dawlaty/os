#include <container_of.h>
#include <lib/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <tests/tests.h>

struct list2c_tester
{
    int num;
    struct list list;
};
#define LIST2C_TESTER_GET(l) containerof(l, struct list2c_tester, list)

#define LIST2C_TESTER_CHECK(t, e) ASSERT_EQ(t->num, e, "%d != %d", t->num, e)
struct list2c_tester *alloc_tester(int num)
{
    struct list2c_tester *out = malloc(sizeof(struct list2c_tester));
    out->list = LIST2C_INIT();
    out->num = num;
    return out;
}

TEST(list2c_test_push_pop, {
    LIST2C_INIT_HEAD(head);

    list2c_push(&alloc_tester(1)->list, &head);
    list2c_push(&alloc_tester(2)->list, &head);
    list2c_push(&alloc_tester(3)->list, &head);

    int i = 3;
    struct list *pos;
    list2c_for_each(pos, &head)
    {
        struct list2c_tester *current = LIST2C_TESTER_GET(pos);
        LIST2C_TESTER_CHECK(current, i);

        i--;
    }

    struct list2c_tester *three = LIST2C_TESTER_GET(list2c_pop(&head));
    struct list2c_tester *two = LIST2C_TESTER_GET(list2c_pop(&head));
    struct list2c_tester *one = LIST2C_TESTER_GET(list2c_pop(&head));

    LIST2C_TESTER_CHECK(three, 3)
    LIST2C_TESTER_CHECK(two, 2)
    LIST2C_TESTER_CHECK(one, 1)
})

TEST(list2c_pop_empty, {
    LIST2C_INIT_HEAD(head);
    struct list *should_be_null = list2c_pop(&head);
    ASSERT_EQ(should_be_null, NULL, "pop on empty list is not null");

    list2c_push(&alloc_tester(1)->list, &head);
    struct list2c_tester *one = LIST2C_TESTER_GET(list2c_pop(&head));
    LIST2C_TESTER_CHECK(one, 1)

    should_be_null = list2c_pop(&head);
    ASSERT_EQ(should_be_null, NULL,
              "pop on empty list after operation is not null");
})
