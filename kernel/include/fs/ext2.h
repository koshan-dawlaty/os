#ifndef EXT2_H
#define EXT2_H

#include <stddef.h>
#include <stdint.h>

typedef struct __attribute__((packed))
{

    uint32_t block_usage;
    uint32_t inode_usage;
    uint32_t inode_table;
    uint16_t unallocated_blocks;
    uint16_t unallocated_inodes;
    uint16_t directories;
    uint8_t unused[14];
} block_group_descriptor_t;

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
} inode_t;

typedef size_t inode_addr_t;
// index of inode content in bytes
typedef uint64_t inode_index_t;

// this does not represent the actual dentry structure
typedef struct
{
    inode_addr_t inode;
    char *name;
} dentry_t;

void init_ext2();

void read_inode(inode_t *node, uint8_t *buf, size_t buf_len,
                inode_index_t index);

inode_addr_t find_inode(char *path);

inode_t *get_inode(inode_addr_t inode);

// TODO: make this always inline
inline uint64_t get_inode_size(inode_t *node)
{
    // MAY BREAK
    return node->size_low;
}

#endif
