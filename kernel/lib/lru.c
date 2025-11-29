#include <assert.h>
#include <container_of.h>
#include <lib/list.h>
#include <lib/lru.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

struct lru_entry *lru_add(struct lru_head *head, struct lru_entry *new)
{
    if (head->head == NULL)
    {
        head->head = new;

        return NULL;
    }

    /* TODO: maybe dont iterate over the array */
    size_t i = 0;
    list2 *pos;
    list2 *prev = NULL;
    list2_for_each(pos, &head->head->list)
    {
        prev = pos;
        i++;
    }

    printf("i:%d\n", i);
    ASSERT(i <= head->max + 1, "length should always be less than capacity");

    struct lru_entry *retval = NULL;
    if (i == head->max)
    {

        if (head->end == NULL)
        {

            struct lru_entry *temp_entry =
                containerof(prev, struct lru_entry, list);

            head->end = temp_entry;
            /* head->end->list.prev = prev; */
            /* prev->next = &head->end->list; */
            /* printf("prev:%p\n", head->end->list.prev); */
            /* printf("pos:%p\n", pos); */
        }

        retval = head->end;
        struct list2 *temp = head->end->list.prev;
        head->end->list.prev->next = NULL;
        /* printf("%p\n", head->end); */
        /* printf("%p\n", head->end->list.prev->prev); */

        struct lru_entry *temp_entry =
            containerof(temp, struct lru_entry, list);
        head->end = temp_entry;
    }

    struct lru_entry *temp_head = head->head;
    head->head = new;
    head->head->list.next = &temp_head->list;
    temp_head->list.prev = &new->list;

    /* list2_add(&new->list, &head->head->list); */

    return retval;
}

struct lru_head *new_lru_head(size_t max)
{
    struct lru_head *out = malloc(sizeof(struct lru_head));
    out->max = max;

    out->head = NULL;
    out->end = NULL;

    return out;
}

struct lru_entry *lru_get(struct lru_head *head,
                          bool (*compare)(struct lru_entry *, void *),
                          void *parameter)
{
    struct lru_entry *value = NULL;

    list2 *pos;
    list2_for_each(pos, &head->head->list)
    {
        struct lru_entry *current = containerof(pos, struct lru_entry, list);
        if (compare(current, parameter))
        {
            value = current;
            break;
        }
    }
    if (value == NULL)
    {
        return NULL;
    }

    /* check if it is already the head */
    if (value->list.prev == NULL)
    {
        return value;
    }
    value->list.prev->next = value->list.next;

    return value;
}
