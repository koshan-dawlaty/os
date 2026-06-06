#ifndef LIST_H
#define LIST_H

#include "container_of.h"
#include <stdbool.h>
#include <stddef.h>

/*
 * singly linked list
 */
typedef struct list
{
    struct list *next;
} list;

list new_list();
void list_add(list *new, list *head);

/*
 * returns true if @entry was found in the list, false otherwise
 */
bool list_remove(list *entry, list **head);
list *list_pop(list **head);
void list_push(struct list *new, struct list **head);

#define LIST_INIT(...)                                                         \
    (list)                                                                     \
    {                                                                          \
        .next = NULL                                                           \
    }
#define LIST_ENTRY(ptr, type, member) containerof(ptr, type, member)
#define list_for_each(pos, head) for (pos = head; pos != NULL; pos = pos->next)

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

struct list2c
{
    struct list2c *prev, *next;
};

#define LIST2C_IS_EMPTY(head) (head.next == &head)
#define LIST2C_HEAD_INIT(name) ((struct list2c){.prev = name, .next = name})
#define LIST2C_INIT_HEAD(name)                                                 \
    struct list2c name;                                                        \
    name = LIST2C_HEAD_INIT(&name);

static inline void list2c_init_head(struct list2c *head)
{
    *head = LIST2C_HEAD_INIT(head);
}

#define LIST2C_DEFINE_HEAD(name) struct list2c name = LIST2C_HEAD_INIT(&name);
#define LIST2C_INIT() ((struct list2c){.prev = NULL, .next = NULL})

void list2c_push(struct list2c *new, struct list2c *head);
void list2c_push_end(struct list2c *new, struct list2c *head);
struct list2c *list2c_pop(struct list2c *head);
struct list2c *list2c_pop_end(struct list2c *head);
// bool list2c_remove(struct list2c *value, struct list2c *head);
void list2c_remove(struct list2c *value);

void list2c_add_after(struct list2c *new, struct list2c *other);
void list2c_add_before(struct list2c *new, struct list2c *other);

#define list2c_for_each(pos, head)                                             \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#endif
