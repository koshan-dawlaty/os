#include "ext2.h"
#include "panic.h"
#include <container_of.h>
#include <err_ptr.h>
#include <errno.h>
#include <kernel/utils.h>
#include <lib/list.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vfs.h>

inode *ext2_get_vfs_node(ext2_inode_addr num, ext2_inode *node, superblock *sb)
{
    return new_inode(num, ext2_get_inode_size(node), node, sb);
}

ext2_inode *ext2_get_inode(ext2_inode_addr inode_number, superblock *sb)
{

    if (inode_number == 0)
    {
        return ERR_PTR(-ENOENT);
    }

    u32 group_index = (inode_number - 1) / EXT2_INODES_PER_GROUP(sb);
    ext2_group_descriptor group = EXT2_GROUP_DESCRIPTOR_TABLE(sb)[group_index];

    /* printf("reading first block\n"); */
    u8 *inode_usage = ext2_read_block(group.inode_usage, sb);

    u8 node_exists = ((inode_usage[(inode_number - 1) / 8]) &
                      (1 << (inode_number - 1) % 8)) != 0;

    if (!node_exists)
    {
        return ERR_PTR(-ENOENT);
    }

    u32 node_index = (inode_number - 1) % EXT2_INODES_PER_GROUP(sb);

    u32 table_index = group.inode_table;

    u8 *table_ptr =
        ext2_read_block(((table_index) + ((node_index * EXT2_INODE_SIZE(sb)) /
                                          EXT2_BLOCK_SIZE(sb))),
                        sb);

    ext2_inode *node =
        (ext2_inode *)(table_ptr + ((node_index * EXT2_INODE_SIZE(sb)) %
                                    EXT2_BLOCK_SIZE(sb)));
    ext2_inode *node_out = malloc(sizeof(ext2_inode));

    memcpy(node_out, node, sizeof(ext2_inode));

    free(table_ptr);

    return node_out;
}

/* gets the actual block number from a logical block */
static u32 ext2_inode_get_block(struct ext2_inode *node, size_t block_num,
                                struct superblock *sb)
{

    if (block_num < 12)
    {
        return node->direct_blocks[block_num];
    }

    if (block_num > EXT2_BLOCK_SIZE(sb))
    {
        panic("ext2 file read too big");
    }

    u32 *indirect_block =
        (u32 *)ext2_read_block(node->singly_indirect_block_pointer, sb);

    u32 block = indirect_block[block_num - 12];
    free(indirect_block);
    return block;
}

/* TODO: this would be a lot better if it was inplace */
void ext2_read_inode(ext2_inode *node, uint8_t *buf, size_t buf_len,
                     ext2_inode_index index, superblock *sb)
{

    size_t block_index = index / EXT2_BLOCK_SIZE(sb);
    size_t current_offset = index % EXT2_BLOCK_SIZE(sb);
    size_t buf_offset = 0;

    while (buf_offset < buf_len)
    {
        u32 block_num = ext2_inode_get_block(node, block_index, sb);
        u8 *block = ext2_read_block(block_num, sb);
        block_index++;

        size_t max_bytes = EXT2_BLOCK_SIZE(sb) - current_offset;
        size_t read_size = buf_len - buf_offset;

        if (read_size > max_bytes)
        {
            read_size = max_bytes;
        }
        memcpy(buf + buf_offset, block + current_offset, read_size);

        free(block);

        buf_offset += read_size;
        current_offset = 0;
    }
}
