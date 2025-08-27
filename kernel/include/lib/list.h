#ifndef LIST_H

/*
 * singly linked list
 */
typedef struct list
{
    struct list *next;
} list;

list new_list();
void list_add(list *new, list *head);

#define LIST_INIT(...)                                                         \
    (list)                                                                     \
    {                                                                          \
        .next = NULL                                                           \
    }
#define LIST_ENTRY(ptr, type, member) containerof(ptr, type, member)
#define list_for_each(pos, head) for (pos = head; pos != NULL; pos = pos->next)

#endif
