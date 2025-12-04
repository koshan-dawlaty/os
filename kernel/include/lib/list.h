#ifndef LIST_H
#define LIST_H

/*
 * singly linked list
 */
typedef struct list
{
    struct list *next;
} list;

list new_list();
void list_add(list *new, list *head);
int list_remove(list *entry, list *head);
list *list_pop(list *head);

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

#endif
