#include "ext2.h"
#include "kernel/types.h"
#include "kernel/utils.h"
#include "panic.h"
#include "utils.h"
#include "vfs.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct inode *ext2_inode_alloc(struct superblock *sb, u16 type)
{
    for (size_t j = 0; j < EXT2_TOTAL_INODES(sb) / EXT2_INODES_PER_GROUP(sb);
         j++)
    {

        for (size_t i = 1; i < EXT2_INODES_PER_GROUP(sb); i++)
        {
            uint32_t usage_block =
                EXT2_SB(sb)->group_descriptor_table->inode_usage;
            uint8_t *usage_bitmap = ext2_read_block(usage_block, sb);

            bool node_exists =
                ((usage_bitmap[(i - 1) / 8]) & (1 << (i - 1) % 8)) != 0;

            if (!node_exists)
            {
                usage_bitmap[(i - 1) / 8] |= (1 << (i - 1) % 8);
                ext2_write_block(usage_bitmap, usage_block, sb);

                inode *node = ext2_iget(i, sb);

                ext2_inode *ext2_node = EXT2_INODE(node);
                ext2_node->type = type;
                /* TODO: only check if the correct bits are set for the type */
                if (type == EXT2_INODE_DIR)
                {
                    /* TODO: this is kinda stupid, but we want to add a dentry
                     * that says that this directory is empty  */
                    ext2_node->size_low = EXT2_BLOCK_SIZE(sb);
                }
                else
                {
                    ext2_node->size_low = 0;
                }
                ext2_update_inode(node, sb);

                free(usage_bitmap);

                return node;
            }
            free(usage_bitmap);
        }
    }

    return NULL;
}

uint32_t ext2_block_alloc(superblock *sb)
{
    for (size_t j = 0; j < EXT2_TOTAL_BLOCKS(sb) / EXT2_BLOCKS_PER_GROUP(sb);
         j++)
    {

        for (size_t i = 1; i < EXT2_BLOCKS_PER_GROUP(sb); i++)
        {
            uint32_t usage_block =
                EXT2_SB(sb)->group_descriptor_table->block_usage;
            uint8_t *usage_bitmap = ext2_read_block(usage_block, sb);

            bool block_exists =
                ((usage_bitmap[(i - 1) / 8]) & (1 << (i - 1) % 8)) != 0;

            if (!block_exists)
            {
                usage_bitmap[(i - 1) / 8] |= (1 << (i - 1) % 8);
                ext2_write_block(usage_bitmap, usage_block, sb);
                free(usage_bitmap);

                return i;
            }
            free(usage_bitmap);
        }
    }

    return 0;
}

/* TODO: this does not update all other refrences to this node in memory */
/*
 * replaces node on disk with node in memory, uses node->number for inode
 * address
 */
void ext2_update_inode(inode *node, superblock *sb)
{
    uint32_t group_index = (node->number - 1) / EXT2_INODES_PER_GROUP(sb);
    ext2_group_descriptor group = EXT2_GROUP_DESCRIPTOR_TABLE(sb)[group_index];
    uint32_t node_index = (node->number - 1) % EXT2_INODES_PER_GROUP(sb);

    ext2_inode *fs_node = EXT2_INODE(node);

    uint32_t table_num =
        group.inode_table +
        ((node_index * EXT2_INODE_SIZE(sb)) / EXT2_BLOCK_SIZE(sb));

    uint8_t *table_block = ext2_read_block(table_num, sb);

    *(ext2_inode *)(table_block + ((node_index * EXT2_INODE_SIZE(sb)) %
                                   EXT2_BLOCK_SIZE(sb))) =
        *((ext2_inode *)node->fs_private);

    node->size = ext2_get_inode_size(fs_node);

    ext2_write_block(table_block, table_num, sb);
}

static inline uint32_t
ext2_get_block_from_inode(inode *vfs_node, uint8_t block_index, superblock *sb)
{

    ext2_inode *node = EXT2_INODE(vfs_node);

    if (node->direct_blocks[block_index] == 0)
    {
        uint32_t block = ext2_block_alloc(sb);
        node->direct_blocks[block_index] = block;
        ext2_update_inode(vfs_node, sb);
        return block;
    }

    return node->direct_blocks[block_index];
}

void ext2_write_inode(inode *vfs_node, const uint8_t *buf, size_t buf_len,
                      ext2_inode_index index, superblock *sb)
{

    ext2_inode *node = EXT2_INODE(vfs_node);

    if (index < EXT2_BLOCK_SIZE(sb) * 12)
    {
        uint block_index = index / EXT2_BLOCK_SIZE(sb);
        uint block_num = ext2_get_block_from_inode(vfs_node, block_index, sb);

        uint8_t *current_block = ext2_read_block(block_num, sb);
        int block_offset = index % EXT2_BLOCK_SIZE(sb);
        /* printf("offset:%d\n", block_offset); */

        if (buf_len <= EXT2_BLOCK_SIZE(sb))
        {
            memcpy(current_block + block_offset, buf, buf_len);
            ext2_write_block(current_block, block_num, sb);

            goto end;
        }

        int write_size = EXT2_BLOCK_SIZE(sb) - (index % EXT2_BLOCK_SIZE(sb));
        memcpy(current_block + block_offset, buf, write_size);
        ext2_write_block(current_block, block_num, sb);

        size_t buf_offset = write_size;
        block_index++;
        current_block = ext2_read_block(
            ext2_get_block_from_inode(vfs_node, block_index, sb), sb);

        while (buf_offset <
               ((buf_len / EXT2_BLOCK_SIZE(sb)) * EXT2_BLOCK_SIZE(sb)))
        {
            memcpy(current_block, buf + buf_offset, EXT2_BLOCK_SIZE(sb));
            ext2_write_block(current_block, node->direct_blocks[block_index],
                             sb);

            block_index++;

            current_block = ext2_read_block(
                ext2_get_block_from_inode(vfs_node, block_index, sb), sb);
            buf_offset += EXT2_BLOCK_SIZE(sb);
        }

        uint bytes_remaining = buf_len % EXT2_BLOCK_SIZE(sb);

        current_block = ext2_read_block(
            ext2_get_block_from_inode(vfs_node, block_index, sb), sb);
        memcpy(current_block, buf + buf_offset, bytes_remaining);

        ext2_write_block(current_block, node->direct_blocks[block_index], sb);
    }
    else
    {
        panic("not implemented!!!!!!!!!! (ext2)\n");
    }

end:;
    uint64_t size = ext2_get_inode_size(node);
    /* printf("%d, %d\n", node->direct_blocks[0], node->direct_blocks[1]); */

    if (index + buf_len > size)
    {
        /* WILL BREAK (maybe ??? ) */
        node->size_low = index + buf_len;
        ext2_update_inode(vfs_node, sb);
    }
}

struct dentry *ext2_append_dentry(struct dentry *parent, const char *name,
                                  enum dentry_type type, superblock *sb)
{

    /* TODO: this will not work if the size goes beyond one block */
    ext2_inode *ext2_entry = EXT2_INODE(parent->inode);

    u8 *buf = malloc(EXT2_BLOCK_SIZE(sb));
    ext2_read_inode(ext2_entry, buf, EXT2_BLOCK_SIZE(sb), 0, sb);
    size_t i = 0;

    /* TODO: loop is bad */
    while (i < EXT2_BLOCK_SIZE(sb))
    {
        u8 *raw_entry = buf + i;

        u16 len = *(u16 *)(raw_entry + 4);
        if (len == 0)
        {
            break;
        }

        if (i + len == EXT2_BLOCK_SIZE(sb))
        {

            size_t new_len = align_long(8 + (*(u8 *)(raw_entry + 6)), 4);

            i += new_len;
            *(u16 *)(raw_entry + 4) = new_len;

            break;
        }
        /* printf("name:%s\n", raw_entry + 8); */

        i += len;
    }

    size_t buf_len = align_long((strlen(name) + 8), 4);
    u8 *new_buf = buf + i;

    struct inode *new_node = ext2_inode_alloc(sb, ext2_get_inode_type(type));

    memcpy(new_buf + 8, name, strlen(name));
    *(u32 *)(new_buf) = new_node->number;
    *(u16 *)(new_buf + 4) = EXT2_BLOCK_SIZE(sb) - i;
    *(u8 *)(new_buf + 6) = strlen(name);

    /* ext2_write_inode(parent->inode, (u8 *)new_buf, buf_len, i, sb); */
    ext2_write_inode(parent->inode, buf, EXT2_BLOCK_SIZE(sb), 0, sb);

    /* TODO: bad */
    ext2_entry->size_low = EXT2_BLOCK_SIZE(sb);
    ext2_update_inode(parent->inode, sb);

    /* TODO: cast discards qualifiers */
    return new_dentry((char *)name, parent, new_node, type);
}
