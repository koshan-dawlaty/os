#include <container_of.h>
#include <lib/list.h>
#include <lib/lru.h>
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

    list2 *pos;

    /* TODO: maybe dont iterate over the array */
    size_t i = 0;
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

/* struct lru_entry *lru_get() */
/* { */
/* } */
