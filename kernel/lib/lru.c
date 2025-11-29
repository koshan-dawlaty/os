#include <container_of.h>
#include <lib/list.h>
#include <lib/lru.h>
#include <stdbool.h>
#include <stdlib.h>

struct lru_entry *lru_add(struct lru_head *head, struct lru_entry *new)
{
    if (head->head == NULL)
    {
        head->head = new;
        return NULL;
    }

    struct lru_entry *temp_head = head->head;
    head->head = new;
    head->head->list.next = &temp_head->list;

    /* TODO: maybe dont iterate over the array */
    size_t i = 0;
    list2 *pos;
    list2_for_each(pos, &head->head->list)
    {
        i++;
    }

    struct lru_entry *retval = NULL;
    if (i >= head->max)
    {
        retval = head->end;
        struct list2 *temp = head->end->list.prev;
        head->end->list.prev->next = NULL;

        struct lru_entry *temp_entry =
            containerof(temp, struct lru_entry, list);
        head->end = temp_entry;
    }

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
