#ifndef BINARY_TREE_H
#define BINARY_TREE_H

struct bintree_node
{
    struct bintree_node *l;
    struct bintree_node *r;
    long value;
};

#define BINTREE_NODE_INIT(_value)                                              \
    (struct bintree_node)                                                      \
    {                                                                          \
        .l = NULL, .r = NULL, .value = _value                                  \
    }

#define BINTREE_HEAD_INIT()                                                    \
    (struct bintree_head)                                                      \
    {                                                                          \
        .root = NULL                                                           \
    }

struct bintree_head
{
    struct bintree_node *root;
};

void bintree_add(struct bintree_head *head, struct bintree_node *new);
struct bintree_node *bintree_get(struct bintree_head *head, long val);

#endif
