#include <lib/binary_tree.h>
#include <tests/tests.h>

TEST(binary_tree_test_basic, {
    struct bintree_head *head = bintree_head_init();
    bintree_add(head, bintree_node_init(10));
    bintree_add(head, bintree_node_init(9));
    bintree_add(head, bintree_node_init(8));
    bintree_add(head, bintree_node_init(7));

    bintree_add(head, bintree_node_init(11));
    bintree_add(head, bintree_node_init(12));
    bintree_add(head, bintree_node_init(13));

    struct bintree_node *root = bintree_get(head, 10);
    ASSERT_EQ(root->value, 10, "getting %d from tree should be equal to",
              root->value, 10);
})
