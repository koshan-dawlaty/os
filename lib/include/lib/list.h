#ifndef LIST_H
#define LIST_H

#include "container_of.h"
#include <stdbool.h>
#include <stddef.h>

/*
 * singly linked list
 */
typedef struct list1_old
{
    struct list1_old *next;
} list1_old;

list1_old new_list1_old();
void list1_old_add(list1_old *new, list1_old *head);

/*
 * returns true if @entry was found in the list, false otherwise
 */
bool list1_old_remove(list1_old *entry, list1_old **head);
list1_old *list1_old_pop(list1_old **head);
void list1_old_push(struct list1_old *new, struct list1_old **head);

#define LIST1_OLD_INIT(...)                                                    \
    (list1_old)                                                                \
    {                                                                          \
        .next = NULL                                                           \
    }
#define LIST_ENTRY(ptr, type, member) containerof(ptr, type, member)
#define list1_old_for_each(pos, head)                                          \
    for (pos = head; pos != NULL; pos = pos->next)

/*
 * doubly linked list
 */
typedef struct list2
{
    struct list2 *prev, *next;
} list2;

#define LIST2_INIT(...)                                                        \
    (list2)                                                                    \
    {                                                                          \
        .prev = NULL, .next = NULL                                             \
    }

#define LIST2_EMPTY(head)                                                      \
    (list2)                                                                    \
    {                                                                          \
        .prev = head, .next = head                                             \
    }

#define list2_for_each(pos, head) for (pos = head; pos != NULL; pos = pos->next)

/*
 * adds new somewhere in the list
 */
void list2_add(list2 *new, list2 *head);
void list2_add_front(list2 *new, list2 **head);
void list2_remove(list2 *value, list2 **head);
void list2_insert_after(list2 *new, list2 *after);
void list2_insert_end(list2 *new, list2 **head);

struct list2 *list2_pop(struct list2 **head);

struct list
{
    struct list *prev, *next;
};

#define LIST_IS_EMPTY(head) (head.next == &head)
#define LIST_HEAD_INIT(name) ((struct list){.prev = name, .next = name})
#define LIST_INIT_HEAD(name)                                                   \
    struct list name;                                                          \
    name = LIST_HEAD_INIT(&name);

static inline void list_init_head(struct list *head)
{
    *head = LIST_HEAD_INIT(head);
}

#define LIST_DEFINE_HEAD(name) struct list name = LIST_HEAD_INIT(&name);
#define LIST_INIT() ((struct list){.prev = NULL, .next = NULL})

void list_push(struct list *new, struct list *head);
void list_push_end(struct list *new, struct list *head);
struct list *list_pop(struct list *head);
struct list *list_pop_end(struct list *head);
void list_remove(struct list *value);

void list_add_after(struct list *new, struct list *other);
void list_add_before(struct list *new, struct list *other);

#define list_for_each(pos, head)                                               \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#endif
