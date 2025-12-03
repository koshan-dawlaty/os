#include <stddef.h>

struct bintree_node
{
    struct bintree_node *l;
    struct bintree_node *r;
    long value;
};

struct bintree_head
{
    struct bintree_node *root;
};

void bintree_add(struct bintree_head *head, struct bintree_node *new)
{
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

    return prev;
}
