#ifndef LRU_H
#define LRU_H

#include <container_of.h>
#include <lib/list.h>
#include <stdbool.h>
#include <stddef.h>

struct lru_entry
{
    struct list2 list;
};

struct lru_head
{
    struct lru_entry *head;
    struct lru_entry *end;
    size_t max;
};

#define LRU_ENTRY_INIT(...)                                                    \
    (struct lru_entry)                                                         \
    {                                                                          \
        .list = LIST2_INIT()                                                   \
    }

struct lru_head *new_lru_head(size_t max);
struct lru_entry *lru_add(struct lru_head *head, struct lru_entry *new);

struct lru_entry *lru_get(struct lru_head *head,
                          bool (*compare)(struct lru_entry *, void *),
                          void *parameter);

/*
 * used to get a lru from a list2 that was gotten from a struct lru_entry
 * ptr should be a struct list2 *
 */
#define LRU_LIST_ENTRY(ptr, type, member)                                      \
    containerof(containerof(ptr, struct lru_entry, list), type, member)

/* TODO: maybe lru for each macro? */

#endif
