#include "bdev.h"
#include "ext2.h"
#include "string.h"
#include <ata/ata.h>
#include <kernel/utils.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs.h>

void ext2_iread(inode *node, uint8_t *buf, size_t buf_len, size_t index,
                superblock *sb)
{
    ext2_read_inode(node->fs_private, buf, buf_len, index, sb);
}

void ext2_iwrite(inode *node, uint8_t *buf, size_t buf_len, size_t offset,
                 superblock *sb)
{
    ext2_write_inode(node, buf, buf_len, offset, sb);
}

inode *ext2_iget(ino_t number, superblock *sb)
{

    ext2_inode *ext2 = ext2_get_inode(number, sb);
    inode *out = new_inode(number, ext2_get_inode_size(ext2), ext2);

    return out;
}

inode *ext2_ialloc(superblock *sb)
{
    ino_t num = ext2_inode_alloc(sb);
    return ext2_iget(num, sb);
}

struct ext2_superblock *ext2_parse_superblock();
superblock *ext2_parse_superblock_vfs();
dentry *ext2_mount();

const struct fs_type ext2_fs_type = {
    .name = "ext2",
    .inode_functions = {.get_inode = ext2_iget,
                        .read_inode = ext2_iread,
                        .write_inode = ext2_iwrite,
                        .alloc_inode = ext2_ialloc},
    .dentry_functions = {.get_dentries = ext2_get_dentries,
                         .append_dentry = ext2_append_dentry,
                         .remove_dentry = ext2_remove_dentry},
    .superblock_functions = {.parse_superblock = ext2_parse_superblock_vfs},

    .mount = ext2_mount,
};

superblock *ext2_parse_superblock_vfs()
{

    struct ext2_superblock *ext2_sb = ext2_parse_superblock();

    superblock *out =
        new_superblock(ext2_sb->block_size, ext2_sb, &ext2_fs_type);

    return out;
}

struct bdev *ext2_drive = NULL;

void fs_read(uint8_t *buffer, size_t buffer_size, uint64_t addr)
{
    // TODO: dynamically select drive or something
    bdev_read(ext2_drive, buffer, buffer_size, addr);
}

void fs_write(uint8_t *buffer, size_t len, uint64_t addr)
{
    bdev_write(ext2_drive, buffer, len, addr);
}

uint8_t *ext2_read_block(size_t index, superblock *sb)
{

    u8 *buffer = malloc(EXT2_BLOCK_SIZE(sb));
    u64 offset = index * EXT2_BLOCK_SIZE(sb);
    fs_read(buffer, EXT2_BLOCK_SIZE(sb), offset);

    return buffer;
}

void ext2_write_block(uint8_t *buffer, size_t index, superblock *sb)
{
    uint64_t offset = (index * (EXT2_BLOCK_SIZE(sb)));
    fs_write(buffer, EXT2_BLOCK_SIZE(sb), offset);
}

static inline uint32_t get_datablock_field(uint8_t *superblock, size_t index)
{
    return bytes_to_32_bit_int(superblock[index], superblock[index + 1],
                               superblock[index + 2], superblock[index + 3]);
}

/*
 * modifies the superblock parameter
 */
struct ext2_superblock *ext2_parse_superblock()
{
    struct ext2_superblock *sb = malloc(sizeof(struct ext2_superblock));
    uint8_t *superblock_buffer = malloc(1024);

    fs_read(superblock_buffer, 1024, 1024);

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

    uint8_t *buf = malloc(block_group_table_size_bytes);
    /* printf("block group:%p\n", block_group_table_size_bytes); */
    /* lba may break with block sizes > 1024 */
    fs_read(buf, block_group_table_size_bytes, 1024 * 2);
    sb->group_descriptor_table = (ext2_group_descriptor *)buf;

    return sb;
}

dentry *ext2_mount()
{
    struct ext2_superblock *ext2_sb = malloc(sizeof(struct ext2_superblock));

    ext2_drive = get_ata_drive(0);
    ext2_sb = ext2_parse_superblock();

    superblock *sb =
        new_superblock(ext2_sb->block_size, ext2_sb, &ext2_fs_type);

    /* TODO: fix */
    return NULL;
}
