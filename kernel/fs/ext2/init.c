#include "bdev.h"
#include "err.h"
#include "ext2.h"
#include "panic.h"
#include "string.h"
#include <ata/ata.h>
#include <kernel/utils.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs.h>

void ext2_iread(inode *node, uint8_t *buf, size_t buf_len, size_t index)
{
    ext2_read_inode(node->fs_private, buf, buf_len, index, node->sb);
}

void ext2_iwrite(inode *node, const uint8_t *buf, size_t buf_len, size_t offset)
{
    ext2_write_inode(node, buf, buf_len, offset, node->sb);
}

inode *ext2_iget(ino_t number, superblock *sb)
{

    /* printf("very start of ext2 iget\n"); */
    ext2_inode *ext2 = ext2_get_inode(number, sb);
    /* printf("got actual inode\n"); */

    if (IS_ERR(ext2))
    {
    }

    /* printf("creating a vfs inode\n"); */
    inode *out = new_inode(number, ext2_get_inode_size(ext2), ext2, sb);
    /* printf("done creating a vfs inode\n"); */

    return out;
}

struct ext2_superblock *ext2_parse_superblock(struct bdev *dev);
superblock *ext2_parse_superblock_vfs(struct bdev *dev);
dentry *ext2_mount(struct bdev *dev);

void ext2_inode_free(struct inode *node)
{
    ext2_inode *ext2 = EXT2_INODE(node);
    free(ext2);
}

const struct fs_type ext2_fs_type = {
    .name = "ext2",
    .inode_functions = {.get_inode = ext2_iget,
                        .read_inode = ext2_iread,
                        .write_inode = ext2_iwrite,
                        .inode_free = ext2_inode_free},
    .dentry_functions = {.append_dentry = ext2_append_dentry,
                         .read_dir = ext2_readdir,
                         .remove_dentry = ext2_remove_dentry},
    .superblock_functions = {.parse_superblock = ext2_parse_superblock_vfs},

    .mount = ext2_mount,
};

superblock *ext2_parse_superblock_vfs(struct bdev *dev)
{

    struct ext2_superblock *ext2_sb = ext2_parse_superblock(dev);

    superblock *out =
        new_superblock(ext2_sb->block_size, ext2_sb, dev, &ext2_fs_type);

    return out;
}

u8 *ext2_read_block(size_t index, superblock *sb)
{

    u8 *buffer = malloc(EXT2_BLOCK_SIZE(sb));
    u64 offset = index * EXT2_BLOCK_SIZE(sb);
    bdev_read(sb->dev, buffer, EXT2_BLOCK_SIZE(sb), offset);

    return buffer;
}

void ext2_write_block(u8 *buffer, size_t index, superblock *sb)
{

    u64 offset = (index * (EXT2_BLOCK_SIZE(sb)));
    bdev_write(sb->dev, buffer, EXT2_BLOCK_SIZE(sb), offset);
}

static inline uint32_t get_datablock_field(uint8_t *superblock, size_t index)
{
    return bytes_to_32_bit_int(superblock[index], superblock[index + 1],
                               superblock[index + 2], superblock[index + 3]);
}

struct ext2_superblock *ext2_parse_superblock(struct bdev *dev)
{
    struct ext2_superblock *sb = malloc(sizeof(struct ext2_superblock));
    uint8_t *superblock_buffer = malloc(1024);

    bdev_read(dev, superblock_buffer, 1024, 1024);

    sb->total_inodes = get_datablock_field(superblock_buffer, 0);
    sb->total_blocks = get_datablock_field(superblock_buffer, 4);
    sb->block_size = 1024 << get_datablock_field(superblock_buffer, 24);
    sb->inodes_per_group = get_datablock_field(superblock_buffer, 40);
    sb->blocks_per_group = get_datablock_field(superblock_buffer, 32);
    sb->fs_version = (superblock_buffer[62]) | (superblock_buffer[63] << 8);
    sb->fs_version |= (get_datablock_field(superblock_buffer, 76) << 16);

    if (sb->fs_version >= 0x1000)
    {
        sb->inode_size = get_datablock_field(superblock_buffer, 88);
        sb->feature_flags_required = get_datablock_field(superblock_buffer, 96);
    }
    else
    {
        sb->inode_size = 128;
    }

    const size_t block_group_size = 32;

    size_t num_block_groups = (sb->total_inodes / sb->inodes_per_group) + 1;
    size_t block_group_table_size =
        ((num_block_groups * block_group_size) / sb->block_size) + 1;

    size_t block_group_table_size_bytes =
        ((block_group_table_size * sb->block_size));
    for (size_t i = 0; i < 100; i++)
    {
        /* printf("%p ", superblock_buffer[i]); */
    }

    uint8_t *buf = malloc(block_group_table_size_bytes);
    /* printf("block group:%p\n", block_group_table_size_bytes); */

    bdev_read(dev, buf, block_group_table_size_bytes, 1024 * 2);

    for (size_t i = 0; i < 100; i++)
    {
        /* printf("%p ", buf[i]); */
    }

    sb->group_descriptor_table = (ext2_group_descriptor *)buf;

    return sb;
}

struct dentry *ext2_mount(struct bdev *dev)
{

    struct ext2_superblock *ext2_sb = ext2_parse_superblock(dev);

    superblock *sb =
        new_superblock(ext2_sb->block_size, ext2_sb, dev, &ext2_fs_type);

    inode *node = ext2_iget(2, sb);

    struct dentry *out = dentry_mount_point_alloc(sb, node);
    return out;
}
