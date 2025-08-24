#ifndef EXT2_H
#define EXT2_H
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <vfs.h>

extern const struct fs_type ext2_fs_type;

typedef struct __attribute__((packed))
{

    uint32_t block_usage;
    uint32_t inode_usage;
    uint32_t inode_table;
    uint16_t unallocated_blocks;
    uint16_t unallocated_inodes;
    uint16_t directories;
    uint8_t unused[14];
} ext2_group_descriptor;

struct ext2_superblock
{

    uint32_t total_inodes;
    uint32_t total_blocks;
    uint16_t inode_size;
    uint32_t block_size;
    uint32_t inodes_per_group;
    uint32_t blocks_per_group;

    uint32_t feature_flags_required;
    uint64_t fs_version;

    ext2_group_descriptor *group_descriptor_table;
};

inline struct ext2_superblock *EXT2_SB(superblock *sb)
{
    return sb->fs_private;
}

#define EXT2_TOTAL_INODES(sb) EXT2_SB(sb)->total_inodes
#define EXT2_TOTAL_BLOCKS(sb) EXT2_SB(sb)->total_blocks
#define EXT2_INODE_SIZE(sb) EXT2_SB(sb)->inode_size
#define EXT2_BLOCK_SIZE(sb) EXT2_SB(sb)->block_size
#define EXT2_INODES_PER_GROUP(sb) EXT2_SB(sb)->inodes_per_group
#define EXT2_BLOCKS_PER_GROUP(sb) EXT2_SB(sb)->blocks_per_group
#define EXT2_VERSION(sb) EXT2_SB(sb)->fs_version

#define EXT2_FEATURE_FLAGS_REQUIRED(sb) EXT2_SB(sb)->feature_flags_required

#define EXT2_GROUP_DESCRIPTOR_TABLE(sb) EXT2_SB(sb)->group_descriptor_table

typedef struct __attribute__((packed))
{
    uint16_t type;
    uint16_t user_id;
    uint32_t size_low;
    uint32_t last_access_time;
    uint32_t creation_time;
    uint32_t modification_time;
    uint32_t deletion_time;
    uint16_t group_id;
    uint16_t hard_links; // how many directories point to this
    uint32_t sectors;    // how many sectors are being used
    uint32_t flags;
    uint32_t os_specific1;
    uint32_t direct_blocks[12];
    uint32_t singly_indirect_block_pointer;
    uint32_t doubly_indirect_block_pointer;
    uint32_t triply_indirect_block_pointer;
    uint32_t generation_number;
    uint32_t extendend_attribute_block;
    uint32_t size_high;
    uint32_t block_addr;
    uint8_t os_specific2[12];
} ext2_inode;

inline ext2_inode *EXT2_INODE(inode *i)
{
    return i->fs_private;
}

inode *ext2_inode_to_inode(ext2_inode node);

typedef size_t ext2_inode_addr;
// index of inode content in bytes
typedef uint64_t ext2_inode_index;

// this does not represent the actual dentry structure
typedef struct
{
    ext2_inode_addr inode;
    char *name;
} ext2_dentry;

void ext2_read_inode(ext2_inode *node, uint8_t *buf, size_t buf_len,
                     ext2_inode_index index, superblock *sb);

ext2_inode_addr find_inode(char *path);

ext2_inode *ext2_get_inode(ext2_inode_addr inode, superblock *sb);

inline uint64_t ext2_get_inode_size(ext2_inode *node)
{
    // MAY BREAK
    return node->size_low;
}

uint8_t *ext2_read_block(size_t block_number, superblock *sb);
void ext2_write_block(uint8_t *buffer, size_t block_number, superblock *sb);

// name is taken from the linux kernel, it could be better
inode *ext2_iget(ino_t inode_number, superblock *sb);
ext2_dentry *ext2_find_dentry(char *path, superblock *sb);

dentry *ext2_dentry_to_dentry(ext2_dentry *dentry, superblock *sb);

ext2_inode_addr ext2_inode_alloc(superblock *sb);
uint32_t ext2_block_alloc(superblock *sb);

void ext2_write_inode(inode *node, uint8_t *buf, size_t buf_len,
                      ext2_inode_index index, superblock *sb);
#endif
