#ifndef LRU_H
#define LRU_H

#include <lib/list.h>
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

#define LRU_HEAD_INIT(_max)                                                    \
    (struct lru_head)                                                          \
    {                                                                          \
        .head = NULL, .end = NULL, .max = _max                                 \
    }

#define LRU_ENTRY_INIT(...)                                                    \
    (struct lru_entry)                                                         \
    {                                                                          \
        .list = LIST2_INIT()                                                   \
    }

struct lru_entry *add_lru(struct lru_head *head, struct lru_entry *new);

#endif
