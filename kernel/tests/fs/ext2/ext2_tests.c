#include "fs/ext2.h"
#include "tests/tests.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

TEST(ext2_read_from_root, {
    const uint8_t *expected = (uint8_t *)"file content\n";
    inode_addr_t node_addr = find_inode("/test.txt");
    inode_t *node = get_inode(node_addr);

    size_t node_size = get_inode_size(node);

    uint8_t *buf = malloc(node_size);

    read_inode(node, buf, node_size, 0);

    for (size_t i = 0; i < node_size; i++)
    {
        ASSERT_EQ(expected[i], buf[i]);
    }
})
