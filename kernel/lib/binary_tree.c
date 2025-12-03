#include <lib/binary_tree.h>
#include <stddef.h>
#include <stdlib.h>

struct bintree_head *bintree_head_init()
{
    struct bintree_head *out = malloc(sizeof(struct bintree_head));
    out->root = NULL;
    return out;
}

void bintree_add(struct bintree_head *head, struct bintree_node *new)
{
    if (head->root == NULL)
    {
        head->root = new;
        return;
    }

    long val = new->value;
    struct bintree_node *current = head->root;
    struct bintree_node *prev = current;

    while (current != NULL)
    {
        prev = current;
        if (current->value < val)
        {
            current = current->l;
        }
        else if (current->value >= val)
        {
            current = current->r;
        }
    }

    if (prev->l == NULL)
    {
        prev->l = new;
    }
    else
    {
        prev->r = new;
    }
}

struct bintree_node *bintree_get(struct bintree_head *head, long val)
{
    struct bintree_node *current = head->root;

    while (current != NULL)
    {

        if (current->value < val)
        {
            current = current->l;
        }
        else if (current->value > val)
        {
            current = current->r;
        }
        else if (current->value == val)
        {
            return current;
        }
    }

    return NULL;
}
