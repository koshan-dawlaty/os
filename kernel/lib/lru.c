#include <container_of.h>
#include <lib/list.h>
#include <lib/lru.h>

struct lru_entry *add_lru(struct lru_head *head, struct lru_entry *new)
{
    list2_add(&new->list, &head->head->list);

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

    list2_add(&new->list, &head->head->list);

    return retval;
}
