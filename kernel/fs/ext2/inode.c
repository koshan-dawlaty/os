#include "err.h"
#include "ext2.h"
#include <container_of.h>
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
static u32 ext2_inode_get_block(struct inode *vfs, size_t block_num,
                                struct superblock *sb)
{
    struct ext2_inode *node = EXT2_INODE(vfs);

    if (block_num < 12)
    {
        return node->direct_blocks[block_num];
    }

    u32 *indirect_block =
        (u32 *)ext2_read_block(node->singly_indirect_block_pointer, sb);

    size_t block = *(u32 *)(indirect_block + ((block_num - 12) * sizeof(u32)));

    return block;
}

/*
 * reads the contents of block, as a indirect block
 */
static void ext2_read_inode_block(u32 indirect_block_num, u8 *buf,
                                  size_t buf_len, size_t offset, superblock *sb)
{
    printf("offset:%d, len:%d\n", offset, buf_len);
    printf("INDEIRECT BLOCK NUM:%d\n", indirect_block_num);
    u8 *indirect_block = ext2_read_block(indirect_block_num, sb);

    /* this assumes there are 12 direct blocks */
    size_t block_index = (offset / EXT2_BLOCK_SIZE(sb)) - 12;
    printf("block_index:%d\n", block_index);
    size_t block_num = *(u32 *)(indirect_block + block_index * sizeof(u32));
    printf("block_num:%d\n", block_num);

    /* it seems like this can return incorrect data */
    u8 *block = ext2_read_block(block_num, sb);
    block_index++;

    size_t bytes_read = 0;

    printf("%d\n", buf_len + (offset % EXT2_BLOCK_SIZE(sb)));
    printf("%d\n", offset % EXT2_BLOCK_SIZE(sb));

    if (buf_len + (offset % EXT2_BLOCK_SIZE(sb)) < EXT2_BLOCK_SIZE(sb))
    {
        memcpy(buf, block + (offset % EXT2_BLOCK_SIZE(sb)), buf_len);
        return;
    }

    printf("maybe untested (ext2)\n");

    return;

    size_t read_size = EXT2_BLOCK_SIZE(sb) - (offset % EXT2_BLOCK_SIZE(sb));
    memcpy(buf, block + offset % EXT2_BLOCK_SIZE(sb), read_size);
    bytes_read += EXT2_BLOCK_SIZE(sb);

    /* should maybe just round down here */
    while (bytes_read < ((buf_len / EXT2_BLOCK_SIZE(sb)) * EXT2_BLOCK_SIZE(sb)))
    {
        block = ext2_read_block(block_index, sb);
        block_index++;

        memcpy(buf + bytes_read, block, EXT2_BLOCK_SIZE(sb));
        bytes_read += EXT2_BLOCK_SIZE(sb);
    }

    size_t bytes_remaining = buf_len - read_size;
    block = ext2_read_block(block_index, sb);
    memcpy(buf + bytes_read, block, bytes_remaining);
}

/* TODO: this would be a lot better if it was inplace */
void ext2_read_inode(ext2_inode *node, uint8_t *buf, size_t buf_len,
                     ext2_inode_index index, superblock *sb)
{

    u8 *block;
    size_t buf_offset = 0;

    size_t blocks_offset = 0;
    uint block_index = index / EXT2_BLOCK_SIZE(sb);

    /* printf("%p\n", node->direct_blocks); */
    /* printf("%d\n", node->direct_blocks[block_index]); */

    block =
        ext2_read_block(node->direct_blocks[block_index + blocks_offset], sb);
    blocks_offset++;

    if (buf_len + (index % EXT2_BLOCK_SIZE(sb)) < EXT2_BLOCK_SIZE(sb))
    {
        memcpy(buf + buf_offset, block + index % EXT2_BLOCK_SIZE(sb), buf_len);

        free(block);

        return;
    }

    int read_size = EXT2_BLOCK_SIZE(sb) - (index % EXT2_BLOCK_SIZE(sb));
    memcpy(buf + buf_offset, block + index % EXT2_BLOCK_SIZE(sb), read_size);

    buf_offset += read_size;

    block =
        ext2_read_block(node->direct_blocks[block_index + blocks_offset], sb);
    blocks_offset++;

    while (buf_offset < ((buf_len / EXT2_BLOCK_SIZE(sb)) * EXT2_BLOCK_SIZE(sb)))
    {
        memcpy(buf + buf_offset, block, EXT2_BLOCK_SIZE(sb));
        free(block);
        block = ext2_read_block(
            node->direct_blocks[block_index + blocks_offset], sb);
        blocks_offset++;

        buf_offset += EXT2_BLOCK_SIZE(sb);
    }

    /* uint bytes_remaining = buf_len - read_size; */
    /* uint bytes_remaining = buf_len - buf_offset; */
    uint bytes_remaining = buf_len % EXT2_BLOCK_SIZE(sb);
    if (bytes_remaining < EXT2_BLOCK_SIZE(sb))
    {
        free(block);
        block = ext2_read_block(
            node->direct_blocks[block_index + blocks_offset - 1], sb);

        memcpy(buf + buf_offset, block, bytes_remaining);
        return;
    }

    printf("THE INDEX IS:%d\n", index);
    if (index > EXT2_BLOCK_SIZE(sb) * 12)
    {
        /* TODO: implement */
        /* printf("NOT IMPLEMENTED!!! (ext2)\n"); */

        printf("reading from node:%p\n", node);
        ext2_read_inode_block(node->singly_indirect_block_pointer, buf, buf_len,
                              index, sb);
    }
}
