#include "ata/ata.h"
#include "fs/ext2.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    uint64_t reserved;
    uint32_t block_addr;
    uint8_t os_specific2[12];
} inode_t;

ata_drive *ext2_drive = NULL;

uint32_t total_inodes = 0;
uint16_t inode_size = 0;
uint32_t block_size = 0;
uint32_t inodes_per_group = 0;

uint64_t fs_version = 0;

block_group_descriptor_t *block_group_descriptor_table;

void fs_read(uint8_t *buffer, size_t buffer_size, uint64_t addr)
{
    read_drive(ext2_drive, buffer, buffer_size, addr);
}

typedef size_t block_index_t;
uint8_t *read_block(block_index_t index)
{
    uint8_t *buffer = malloc(block_size);

    uint64_t lba = (index * (block_size / 512));
    printf("block addr: %d, lba: %d\n", index, lba);
    fs_read(buffer, block_size, lba);

    return buffer;
}

// TODO: stupid name
inline uint32_t four_bytes_to_32_bit_int(uint8_t one, uint8_t two,
                                         uint8_t three, uint8_t four)
{
    return (one) | (two << 8) | (three << 16) | (four << 24);
}

inline uint32_t get_datablock_field(uint8_t *superblock, size_t index)
{
    return four_bytes_to_32_bit_int(superblock[index], superblock[index + 1],
                                    superblock[index + 2],
                                    superblock[index + 3]);
}

typedef size_t inode_addr_t;

inode_t get_inode(inode_addr_t inode)
{
    uint32_t group_index = (inode - 1) / inodes_per_group;

    /* block_group_descriptor_t group = */
    /*     block_group_descriptor_table[(inode - 1) / inodes_per_group]; */

    printf("%d\n", inode);
    block_group_descriptor_t group = block_group_descriptor_table[group_index];

    uint8_t *inode_usage = read_block(group.inode_usage);

    for (int i = 0; i < 512; i++)
    {
        // printf("%p ", inode_usage[i]);
    }

    // this may be inode - 1 but it seems to work without it
    uint8_t node_exists =
        ((inode_usage[(inode) / 8]) & (1 << (inode) % 8)) != 0;

    if (!node_exists)
    {
        printf("node not found\n");

        return (inode_t){};
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

    uint32_t node_index = (inode - 1) % inodes_per_group;

    /* uint8_t *node_ptr = read_block((group.inode_table) + */
    /*                                ((node_index * inode_size) / block_size));
     */
    uint32_t table = group.inode_table;

    printf("table:%p\n", table);
    uint8_t *node_ptr =
        read_block(((table) + ((node_index * inode_size) / block_size)));
    printf("reading: %d\n",
           ((table) + ((node_index * inode_size) / block_size)));

    for (size_t i = 0; i < block_size; i++)
    {
        // printf("%p", node_ptr[i]);
    }

    printf("index: %p\n", node_index);

    inode_t *node =
        (inode_t *)(node_ptr + ((node_index * inode_size) % block_size));
    /* uint8_t *node = &node_ptr[node_index * inode_size]; */
    printf("%p\n", node->type);

    printf("group: %d, local: %d\n", group_index, node_index);

    for (int i = 0; i < inode_size; i++)
    {
        printf("%p", ((uint8_t *)(node))[i]);
    }

    return *node;
}

void parse_superblock()
{
    uint8_t *superblock = malloc(1024);
    fs_read(superblock, 1024, 2);

    total_inodes = get_datablock_field(superblock, 0);
    block_size = 1024 << get_datablock_field(superblock, 24);
    inodes_per_group = get_datablock_field(superblock, 40);
    fs_version = (superblock[62]) | (superblock[63] << 8);
    fs_version |= (get_datablock_field(superblock, 76) << 16);

    printf("test: %d\n", get_datablock_field(superblock, 20));

    if (fs_version >= 0x1000)
    {
        inode_size = get_datablock_field(superblock, 88);
    }
    else
    {
        inode_size = 128;
    }
}

void init_ext2()
{

    // FIXME: should be other drives sometimes (maybe) (idk????)
    ext2_drive = get_ata_drive(0);
    parse_superblock();

    block_group_descriptor_table =
        (block_group_descriptor_t *)read_block((block_size / 512));

    inode_t node = get_inode(2);
    printf("\n%p\n", node.type);
}
