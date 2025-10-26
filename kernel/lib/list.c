#include "lib/list.h"
#include <err.h>
#include <stdio.h>
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

int list_remove(list *entry, list *head)
{

    if (entry == head)
    {
        head = head->next;
    }

    list *current = head;
    list *prev = head;
    while (current != NULL)
    {

        if (current == entry)
        {
            prev->next = current->next;
            return 0;
        }

        prev = current;
        current = head->next;
    }

    return 1;
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

/* TODO: bad */
void list2_remove(list2 *value, list2 **head)
{
    if (value->next == NULL && value->prev == NULL)
    {
        *head = NULL;
        return;
    }

    if (value == *head)
    {
        list2 *temp = (*head)->next;
        *head = temp;
        (*head)->prev = NULL;
    }

    if (value->next == NULL)
    {
        value->prev->next = NULL;
        return;
    }

    if (value->prev == NULL)
    {
        value->next->prev = NULL;
        return;
    }

    value->next->prev = value->prev;
    value->prev->next = value->next;
}
