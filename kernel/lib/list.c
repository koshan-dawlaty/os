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
