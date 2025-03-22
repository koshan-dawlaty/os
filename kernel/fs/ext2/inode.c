#include "ext2.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vfs.h>

ext2_inode *ext2_get_inode(ext2_inode_addr inode_number, superblock *sb)
{

    if (inode_number == 0)
    {
        return NULL;
    }

    uint32_t group_index = (inode_number - 1) / EXT2_INODES_PER_GROUP(sb);
    // printf("test\n");

    /* block_group_descriptor_t group = */
    /*     block_group_descriptor_table[(inode - 1) / inodes_per_group]; */

    // printf("%d\n", inode);
    ext2_group_descriptor group = EXT2_GROUP_DESCRIPTOR_TABLE(sb)[group_index];

    // printf("inode usage:%p\n", group.inode_usage);

    uint8_t *inode_usage = ext2_read_block(group.inode_usage, sb);

    for (int i = 0; i < 512; i++)
    {
        // printf("%p ", inode_usage[i]);
    }

    // printf("\n");

    uint8_t node_exists = ((inode_usage[(inode_number - 1) / 8]) &
                           (1 << (inode_number - 1) % 8)) != 0;

    // printf("%p\n", ((inode_usage[(inode) / 8])));
    // printf("%p\n", ((inode_usage[(inode) / 8]) & (1 << (inode - 1 % 8))));
    if (!node_exists)
    {
        // shouldn't print
        printf("node not found\n");

        return NULL;
    }

    /* for (size_t i = 0; i < 512; i++) */
    /* { */

    /*     /1* printf("%p ", inode_usage[i]); *1/ */
    /* } */
    /* void *table = (void *)read_block((group.inode_table)); */
    /* printf("%p\n", *table); */
    /* return table[(inode - 1) % inodes_per_group]; */
    /* void *table = (void *)read_block(group.inode_table + */
    /*                                  ((inode * inode_size) / block_size)); */

    uint32_t node_index = (inode_number - 1) % EXT2_INODES_PER_GROUP(sb);

    /* uint8_t *node_ptr = read_block((group.inode_table) + */
    /*                                ((node_index * inode_size) / block_size));
     */
    uint32_t table_index = group.inode_table;

    uint8_t *table_ptr =
        ext2_read_block(((table_index) + ((node_index * EXT2_INODE_SIZE(sb)) /
                                          EXT2_BLOCK_SIZE(sb))),
                        sb);

    printf("%d\n", table_index);
    printf("reading: %d\n",
           ((table_index) +
            ((node_index * EXT2_INODE_SIZE(sb)) / EXT2_BLOCK_SIZE(sb))));

    for (size_t i = 0; i < EXT2_BLOCK_SIZE(sb); i++)
    {
        // printf("%p", table_ptr[i]);
    }

    ext2_inode *node =
        (ext2_inode *)(table_ptr + ((node_index * EXT2_INODE_SIZE(sb)) %
                                    EXT2_BLOCK_SIZE(sb)));
    ext2_inode *node_out = malloc(sizeof(ext2_inode));

    memcpy(node_out, node, sizeof(ext2_inode));

    free(table_ptr);

    return node_out;
}

void ext2_read_inode(ext2_inode *node, uint8_t *buf, size_t buf_len,
                     ext2_inode_index index, superblock *sb)
{

    uint8_t *block;
    size_t buf_offset = 0;
    if (index < EXT2_BLOCK_SIZE(sb) * 12)
    {
        uint block_index = index / EXT2_BLOCK_SIZE(sb);
        printf("%p\n", node->direct_blocks);
        printf("%d\n", node->direct_blocks[block_index]);
        block = ext2_read_block(node->direct_blocks[block_index], sb);
        int read_size = EXT2_BLOCK_SIZE(sb) - (index % EXT2_BLOCK_SIZE(sb));
        if (buf_len < EXT2_BLOCK_SIZE(sb))
        {
            memcpy(buf + buf_offset, block + index % EXT2_BLOCK_SIZE(sb),
                   buf_len);
            return;
        }
        memcpy(buf + buf_offset, block + index % EXT2_BLOCK_SIZE(sb),
               read_size);
        buf_offset += read_size;

        uint bytes_remaining = buf_len - read_size;
        if (bytes_remaining < EXT2_BLOCK_SIZE(sb))
        {
            block = ext2_read_block(node->direct_blocks[block_index + 1], sb);
            memcpy(buf + buf_offset, block, bytes_remaining);
            return;
        }
    }
    else
    {
        // TODO: implement
        printf("NOT IMPLEMENTED!!! (ext2)\n");
    }
}
