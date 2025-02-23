#include "ata/ata.h"
#include "fs/ext2.h"
#include "sys/types.h"
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
    uint32_t extendend_attribute_block;
    uint32_t size_high;
    uint32_t block_addr;
    uint8_t os_specific2[12];
} inode_t;

// TODO: make this always inline
inline uint64_t get_inode_size(inode_t *node)
{
    // MAY BREAK
    return node->size_low;
}

typedef size_t inode_addr_t;
// index of inode content in bytes
typedef uint64_t inode_index_t;

// this does not represent the actual dentry structure
typedef struct
{
    inode_addr_t inode;
    char *name;
} dentry_t;

ata_drive *ext2_drive = NULL;

uint32_t total_inodes = 0;
uint16_t inode_size = 0;
uint32_t block_size = 0;
uint32_t inodes_per_group = 0;

uint32_t feature_flags_required = 0;

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
    // printf("block addr: %d, lba: %d\n", index, lba);
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

/*
 * returns NULL if node is not found
 */
inode_t *get_inode(inode_addr_t inode)
{
    uint32_t group_index = (inode - 1) / inodes_per_group;

    /* block_group_descriptor_t group = */
    /*     block_group_descriptor_table[(inode - 1) / inodes_per_group]; */

    // printf("%d\n", inode);
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

    uint32_t node_index = (inode - 1) % inodes_per_group;

    /* uint8_t *node_ptr = read_block((group.inode_table) + */
    /*                                ((node_index * inode_size) / block_size));
     */
    uint32_t table_index = group.inode_table;

    uint8_t *table_ptr =
        read_block(((table_index) + ((node_index * inode_size) / block_size)));
    // printf("reading: %d\n",
    //        ((table_index) + ((node_index * inode_size) / block_size)));

    for (size_t i = 0; i < block_size; i++)
    {
        // printf("%p", table_ptr[i]);
    }

    inode_t *node =
        (inode_t *)(table_ptr + ((node_index * inode_size) % block_size));

    inode_t *node_out = malloc(sizeof(inode_t));
    memcpy(node_out, node, sizeof(inode_t));

    free(table_ptr);

    return node_out;
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

    if (fs_version >= 0x1000)
    {
        inode_size = get_datablock_field(superblock, 88);
        feature_flags_required = get_datablock_field(superblock, 96);
    }
    else
    {
        inode_size = 128;
    }
}

void read_inode(inode_t *node, uint8_t *buf, size_t buf_len,
                inode_index_t index)
{
    uint8_t *block;
    size_t buf_offset = 0;
    if (index < block_size * 12)
    {

        uint block_index = index / block_size;
        block = read_block(node->direct_blocks[block_index]);
        int read_size = block_size - (index % block_size);
        if (buf_len < index % block_size)
        {
            memcpy(buf + buf_offset, block + index % block_size, buf_len);
            return;
        }
        memcpy(buf + buf_offset, block + index % block_size, read_size);
        buf_offset += read_size;

        uint bytes_remaining = buf_len - read_size;
        if (bytes_remaining < block_size)
        {
            block = read_block(node->direct_blocks[block_index + 1]);
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

/*
 * this returns a heap pointer to heap pointers
 */
dentry_t **parse_directory(inode_t *node)
{

    // MAY BREAK
    if ((node->type >> 12) != 4)
    {
        // not a directory
        return NULL;
    }

    uint64_t size = get_inode_size(node);

    uint8_t *content = malloc(size);
    // size may be wrong
    read_inode(node, content, size, 0);

    for (size_t i = 0; i < 512; i++)
    {
        // printf("%p ", content[i]);
    }

    dentry_t **out = malloc(sizeof(dentry_t *));
    size_t out_len = 1;

    int dentry_type_present = feature_flags_required & 2;

    uint offset = 0;
    uint name_len = 0;
    while (offset < size)
    {
        uint8_t *ptr = (uint8_t *)(content + offset);
        // MAY BE WRONG
        inode_index_t inode =
            four_bytes_to_32_bit_int(ptr[0], ptr[1], ptr[2], ptr[3]);

        // hopefully the compiler will optimize this into 2 loops
        if (dentry_type_present)
        {
            name_len = (*(ptr + 6));
        }
        else
        {
            // MAY BE WRONG
            name_len = (*(ptr + 6)) | ((*(ptr + 7)) << 8);
        }
        // printf("%d ", name_len);

        char *name = malloc((name_len + 1) * sizeof(char));

        for (uint i = 0; i < name_len; i++)
        {
            // printf("%c", *(ptr + 8 + i));
            name[i] = (*(ptr + 8 + i));
        }
        name[name_len] = '\0';
        // printf("%s\n", name);
        offset += (*(ptr + 4)) | (*(ptr + 5) << 8);
        // printf("%d\n", offset);

        dentry_t *entry = malloc(sizeof(dentry_t));
        entry->inode = inode;
        entry->name = name;

        out[out_len - 1] = entry;

        out_len++;
        out = realloc(out, out_len * sizeof(dentry_t));
    }

    out[out_len] = NULL;

    return out;
}

// this is useless
void parse_rootdir()
{
    inode_t *rootdir = get_inode(2);
    if (rootdir == NULL)
    {
        return;
    }

    dentry_t **entries = parse_directory(rootdir);

    while (*entries != NULL)
    {
        printf("%s\n", (*entries)->name);
        entries++;
    }
}

void init_ext2()
{
    // FIXME: should be other drives sometimes (maybe) (idk????)
    ext2_drive = get_ata_drive(0);
    parse_superblock();

    block_group_descriptor_table =
        (block_group_descriptor_t *)read_block((block_size / 512));

    parse_rootdir();
}
