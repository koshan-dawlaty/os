#include <err_ptr.h>
#include <lib/list.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

list1_old new_list1_old()
{
    list1_old out;
    out.next = NULL;
    return out;
}

list1_old *list1_old_pop(list1_old **head)
{
    list1_old *retval = *head;
    *head = (*head)->next;
    return retval;
}

void list1_old_add(list1_old *new, list1_old *head)
{
    list1_old *temp = head->next;
    head->next = new;
    new->next = temp;
}
void list1_old_push(struct list1_old *new, struct list1_old **head)
{
    struct list1_old *temp = *head;
    *head = new;
    (*head)->next = temp;
}

bool list1_old_remove(list1_old *entry, list1_old **head)
{

    if (entry == *head)
    {
        *head = (*head)->next;
        return true;
    }

    struct list1_old *pos;
    struct list1_old *prev;
    list1_old_for_each(pos, *head)
    {
        if (pos == entry)
        {
            prev->next = pos->next;
            pos->next = NULL;
            return true;
        }

        prev = pos;
    }

    return false;
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

void list2_add_front(list2 *new, list2 **head)
{
    list2 *temp = *head;
    *head = new;

    new->next = temp;
    temp->prev = new;
    new->prev = NULL;
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

void list2_insert_after(list2 *new, list2 *after)
{

    list2 *temp = after->next;

    after->next = new;
    new->prev = after;

    new->next = temp;
    if (temp != NULL)
    {
        temp->prev = new;
    }
}

void list2_insert_end(list2 *new, list2 **head)
{
    if (*head == NULL)
    {
        new = *head;
        return;
    }

    list2 *current = *head;
    list2 *prev = *head;

    while (current != NULL)
    {
        prev = current;
        current = current->next;
    }

    prev->next = new;
    new->prev = prev;
}

struct list2 *list2_pop(struct list2 **head)
{

    struct list2 *retval = *head;
    *head = (*head)->next;

    if (*head != NULL)
    {
        (*head)->prev = NULL;
    }

    return retval;
}

void list2c_push(struct list2c *new, struct list2c *head)
{
    /* printf("PUSH\n"); */
    struct list2c *temp = head->next;
    new->prev = head;
    new->next = temp;

    temp->prev = new;
    head->next = new;
}

void list2c_push_end(struct list2c *new, struct list2c *head)
{
    /* printf("PUSH\n"); */
    struct list2c *temp = head->prev;
    new->next = head;
    new->prev = temp;

    temp->next = new;
    head->prev = new;
}

struct list2c *list2c_pop(struct list2c *head)
{
    struct list2c *out = head->next;

    if (out == head)
    {
        return NULL;
    }

    head->next = out->next;
    head->next->prev = head;

    out->next = NULL;
    out->prev = NULL;

    return out;
}

struct list2c *list2c_pop_end(struct list2c *head)
{
    struct list2c *out = head->prev;

    if (out == head)
    {
        return NULL;
    }

    head->prev = out->prev;
    head->prev->next = head;

    out->next = NULL;
    out->prev = NULL;

    return out;
}

void list2c_remove(struct list2c *value)
{
    /* printf("REMOVE\n"); */
    value->prev->next = value->next;
    value->next->prev = value->prev;

    value->next = NULL;
    value->prev = NULL;
}

void list2c_add_after(struct list2c *new, struct list2c *other)
{
    struct list2c *temp = other->next;
    other->next = new;
    new->next = temp;
    new->prev = other;
    temp->prev = new;
}

void list2c_add_before(struct list2c *new, struct list2c *other)
{
    struct list2c *temp = other->prev;
    other->prev = new;
    new->prev = temp;
    new->next = other;
    temp->next = new;
}
