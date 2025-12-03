#include <lib/binary_tree.h>
#include <stdlib.h>
#include <tests/tests.h>

/* TODO */
/* #define BINTREE_TEST_CHECK_NUM(head, num) \ */
/*     { \ */
/*         struct bintree_node *node = bintree_get(head, num); \ */
/*         if (node == NULL) \ */
/*         { \ */
/*             FAIL_TEST("getting value %d from tree returns null"); \ */
/*         } \ */
/*         else \ */
/*         { \ */
/*             ASSERT_EQ(node->value, num, \ */
/*                       "getting value %d from tree returns %d", num, \ */
/*                       node->value); \ */
/*         } \ */
/*     } */

/* TEST(binary_tree_test_basic, { */
/*     struct bintree_head *head = bintree_head_init(); */
/*     bintree_add(head, bintree_node_init(10)); */
/*     bintree_add(head, bintree_node_init(9)); */
/*     bintree_add(head, bintree_node_init(8)); */
/*     bintree_add(head, bintree_node_init(7)); */

/*     bintree_add(head, bintree_node_init(11)); */
/*     bintree_add(head, bintree_node_init(12)); */
/*     bintree_add(head, bintree_node_init(13)); */

/*     BINTREE_TEST_CHECK_NUM(head, 10); */
/*     BINTREE_TEST_CHECK_NUM(head, 11); */
/*     BINTREE_TEST_CHECK_NUM(head, 8); */
/* }) */
