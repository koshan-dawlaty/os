#include "lib/list.h"
#include <stdlib.h>

list new_list()
{
    list out;
    out.next = NULL;
    return out;
}

void list_add(list *new, list *head)
{
    list *temp = head->next;
    head->next = new;
    new->next = temp;
}

void list2_add(list2 *new, list2 *head)
{
    list2 *temp = head->next;

    if (temp == NULL)
    {
        head->next = new;
        new->next = NULL;
        new->prev = head;
        return;
    }
    head->next = new;
    temp->prev = new;
    new->next = temp;
    new->prev = head;
}

void list2_remove(list2 *value)
{
    value->next->prev = value->prev;
    value->prev->next = value->next;
}
