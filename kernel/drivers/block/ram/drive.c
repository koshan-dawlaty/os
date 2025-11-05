#include "ata/ata.h"
#include "container_of.h"
#include "kernel/paging.h"
#include "lib/list.h"
#include "memory/get_free_pages.h"
#include <bdev.h>
#include <errno.h>
#include <panic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ramdisk_block
{
    void *data;
    list l;
};

#define RAMDISK_BLOCK_PAGES 1

#define RAMDISK_BDEV(bdev) (struct ramdisk_block *)(bdev->bdev_private)

struct ramdisk_block *new_ramdisk_block(struct bdev *dev)
{
    struct ramdisk_block *out = malloc(sizeof(struct ramdisk_block));
    out->l = LIST_INIT();
    out->data = get_free_pages(dev->block_size / PAGE_SIZE);

    return out;
}

int ramdisk_read_blocks(struct bdev *dev, u8 *buf, size_t num_blocks,
                        u64 offset)
{

    struct ramdisk_block *head = RAMDISK_BDEV(dev);

    size_t i = 0;

    /* TODO: this is slow, maybe store pointers to some of the blocks in an
     * array? */
    list *pos = NULL;
    list_for_each(pos, &head->l)
    {
        if (i == offset)
        {
            break;
        }

        i++;
    }

    if (pos == NULL)
    {
        return -EINVAL;
    }

    size_t buf_offset = 0;
    list *current = pos;
    for (size_t j = 0; j < num_blocks; j++)
    {

        struct ramdisk_block *block =
            LIST_ENTRY(current, struct ramdisk_block, l);

        memcpy(buf + buf_offset, block->data, dev->block_size);
        buf_offset += dev->block_size;

        current = current->next;
    }

    return 0;
}

int ramdisk_write_blocks(struct bdev *dev, u8 *buf, size_t num_blocks,
                         u64 block_num)
{

    struct ramdisk_block *head = RAMDISK_BDEV(dev);

    size_t i = 0;

    list *pos = NULL;
    list *prev = &head->l;
    list_for_each(pos, &head->l)
    {
        if (i == block_num)
        {
            break;
        }

        prev = pos;
        i++;
    }

    list *start = pos;

    if (pos == NULL)
    {
        list *current = prev;

        size_t new_blocks = (block_num + num_blocks) - i;

        for (size_t j = 0; j < new_blocks; j++)
        {
            struct ramdisk_block *new = new_ramdisk_block(dev);

            if (j == block_num - i)
            {
                start = current;
            }

            current->next = &new->l;
            current = current->next;
        }
    }

    list *current = start;
    size_t buf_offset = 0;
    for (size_t i = 0; i < num_blocks; i++)
    {

        struct ramdisk_block *block =
            LIST_ENTRY(current, struct ramdisk_block, l);

        memcpy(block->data, buf + buf_offset, dev->block_size);

        buf_offset += dev->block_size;

        current = current->next;
    }

    return 0;
}

void free_ramdisk(struct bdev *dev)
{
    struct ramdisk_block *start_block = RAMDISK_BDEV(dev);
    list *current = &start_block->l;
    list *prev = current;

    while (current != NULL)
    {
        prev = current;
        current = current->next;

        struct ramdisk_block *block = LIST_ENTRY(prev, struct ramdisk_block, l);
        free_pages(block->data, dev->block_size / PAGE_SIZE);
        free(block);
    }
}

struct bdev_functions ramdisk_functions = {.read_blocks = ramdisk_read_blocks,
                                           .write_blocks = ramdisk_write_blocks,
                                           .free = free_ramdisk};

struct bdev *new_ramdisk()
{
    struct ramdisk_block *head = malloc(sizeof(struct ramdisk_block));
    head->data = get_free_pages(RAMDISK_BLOCK_PAGES);
    head->l = LIST_INIT();

    struct bdev *out =
        new_bdev(ramdisk_functions, PAGE_SIZE * RAMDISK_BLOCK_PAGES, head);

    return out;
}
