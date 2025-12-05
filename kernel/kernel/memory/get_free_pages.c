#include "memory/paging_flags.h"
#include <container_of.h>
#include <kernel/paging.h>
#include <kernel/types.h>
#include <lib/binary_tree.h>
#include <lib/list.h>
#include <memory/get_free_pages.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_BITMAP_SIZE 1024 /* size in pages */
u8 gfp_kernel_bitmap[DEFAULT_BITMAP_SIZE / 8] = {GFP_FREE};

struct bitmap_memory_zone gfp_bitmap_kernel_zone = {
#ifdef CONFIG_x86_64
    .virtual = 0xffffD00000000000,
#elif defined(CONFIG_x86_32)
    .virtual = 0xD0000000,
#endif
    .physical = 0x6000000,
    .size = DEFAULT_BITMAP_SIZE,
    .bitmap = gfp_kernel_bitmap,
    .pg_flags = PAGING_WRITEABLE,
};

const struct memory_zone gfp_kernel_zone =
    BITMAP_TO_GFP_ZONE(&gfp_bitmap_kernel_zone);

struct bitmap_memory_zone gfp_bitmap_user_zone = {.virtual = 0x90000000,
                                                  .physical = 0xC00000,
                                                  .size = DEFAULT_BITMAP_SIZE,
                                                  .bitmap = NULL,
                                                  .pg_flags = PAGING_WRITEABLE |
                                                              PAGING_USER};

struct memory_zone gfp_user_zone = BITMAP_TO_GFP_ZONE(&gfp_bitmap_user_zone);

/*
 * @amount: the number of pages to allocate
 * @zone: the zone to allocated the pages from
 *
 * returns address of allocated memory
 */
void *get_free_pages_bitmap(size_t amount, const struct memory_zone *z)
{

    struct bitmap_memory_zone *zone = z->zone_private;

    ptr i;
    size_t consecutive = 0;
    size_t consecutive_start = 0;
    for (i = 0; i < zone->size; i++)
    {
        if (((zone->bitmap[i / 8] >> (i % 8)) & 1) == GFP_FREE)
        {
            /* printf("i is:%d\n", i); */
            if (consecutive == 0)
            {
                consecutive_start = i;
            }

            consecutive++;
        }
        else
        {
            consecutive_start = 0;
            consecutive = 0;
        }

        if (consecutive >= amount)
        {
            break;
        }
    }

    /* printf("start:%d\n", consecutive_start); */

    void *addr = (void *)((i)*PAGE_SIZE);

    void *virtual_addr = addr + zone->virtual;
    void *physical_addr = addr + zone->physical;

    for (size_t j = consecutive_start; j < amount + i; j++)
    {
        /* printf("j is:%d\n", j); */

        /* printf("virtual:%p\nphysical:%p\n", virtual_addr, physical_addr); */

        zone->bitmap[j / 8] |= (1 << (j % 8));

        map_page(physical_addr, virtual_addr, zone->pg_flags);

        /* physical_addr += PAGE_SIZE * j; */
        /* virtual_addr += PAGE_SIZE * j; */

        physical_addr += PAGE_SIZE;
        virtual_addr += PAGE_SIZE;
    }

    void *address = (void *)((i * PAGE_SIZE) + zone->virtual);

    /* release_semaphore(get_free_pages_semaphore); */

    return address;
}

void free_pages_bitmap(void *addr, size_t amount, const struct memory_zone *z)
{
    struct bitmap_memory_zone *zone = z->zone_private;

    size_t index = (((size_t)addr - zone->virtual)) / 4096;

    for (size_t i = 0; i < amount; i++)
    {
        zone->bitmap[(index + i) / 8] |= GFP_FREE >> ((index + i) % 8);
        unmap_page((void *)(addr + (i * PAGE_SIZE)));
    }
}

bool gfp_zones_initialized = false;

void *get_free_pages_unmmaped(size_t amount, const struct memory_zone *z)
{
    struct bitmap_memory_zone *zone = z->zone_private;

    ptr i;
    size_t consecutive = 0;
    size_t consecutive_start = 0;
    for (i = 0; i < zone->size; i++)
    {
        if (((zone->bitmap[i / 8] >> (i % 8)) & 1) == GFP_FREE)
        {
            /* printf("i is:%d\n", i); */
            if (consecutive == 0)
            {
                consecutive_start = i;
            }

            consecutive++;
        }
        else
        {
            consecutive_start = 0;
            consecutive = 0;
        }

        if (consecutive >= amount)
        {
            break;
        }
    }

    /* printf("start:%d\n", consecutive_start); */

    for (size_t j = consecutive_start; j < amount + i; j++)
    {
        /* printf("j is:%d\n", j); */

        /* printf("virtual:%p\nphysical:%p\n", virtual_addr, physical_addr); */

        zone->bitmap[j / 8] |= (1 << (j % 8));

        /* map_page(physical_addr, virtual_addr, zone->pg_flags); */

        /* physical_addr += PAGE_SIZE * j; */
        /* virtual_addr += PAGE_SIZE * j; */
    }

    void *address = (void *)((i * PAGE_SIZE) + zone->virtual);

    /* release_semaphore(get_free_pages_semaphore); */

    return address;
}

/* structure representing an free memory zone in the gfp tree */
struct gfp_tree_value
{
    void *addr;
    struct list list;
};

struct gfp_tree_node
{
    struct bintree_node bintree;
    struct list *values;
};

struct gfp_tree_zone kernel_tree_zone = {};

struct memory_zone gfp_kernel_tree_zone = {
    .get_pages = get_free_pages_tree,
    .free_pages = free_pages_tree,
    .zone_private = NULL,
};

struct gfp_tree_node *gfp_tree_node_init(long value)
{
    struct gfp_tree_node *out = malloc(sizeof(struct gfp_tree_node));

    out->values = NULL;
    out->bintree = BINTREE_NODE_INIT(value);

    return out;
}

struct gfp_tree_value *gfp_tree_value_init(void *addr)
{
    struct gfp_tree_value *out = malloc(sizeof(struct gfp_tree_value));

    out->addr = addr;
    out->list = LIST_INIT();
    return out;
}

void *get_free_pages_tree(size_t amount, const struct memory_zone *z)
{

    struct gfp_tree_zone *zone = z->zone_private;

    struct bintree_node *node = bintree_get(zone->head, amount);

    if (node == NULL)
    {
        node = &gfp_tree_node_init(amount)->bintree;
        bintree_add(zone->head, &gfp_tree_node_init(amount)->bintree);
    }

    struct gfp_tree_node *gfp_node =
        containerof(node, struct gfp_tree_node, bintree);

    if (gfp_node->values == NULL)
    {
        /* TODO: dont use zone->base */
        map_page(get_free_phys_page(), zone->base + (zone->offset * 4096),
                 PAGING_PRESENT | PAGING_WRITEABLE);

        struct gfp_tree_value *value =
            gfp_tree_value_init(zone->base + (zone->offset * 4096));
        zone->offset++;

        gfp_node->values = &value->list;
        return value->addr;
    }
    else
    {
        list *l = list_pop(gfp_node->values);
        struct gfp_tree_value *out =
            containerof(l, struct gfp_tree_value, list);

        return out->addr;
    }
}

void free_pages_tree(void *addr, size_t amount, const struct memory_zone *z)
{
    struct gfp_tree_zone *zone = z->zone_private;

    struct bintree_node *root = bintree_get(zone->head, amount);

    if (root == NULL)
    {
        /* invalid */
        printf("possible double free_pages_tree\n");
        return;
    }

    struct gfp_tree_node *gfp_node =
        containerof(root, struct gfp_tree_node, bintree);

    if (gfp_node->values == NULL)
    {
        /* invalid */
        printf("possible double free_pages_tree\n");
        return;
    }

    struct gfp_tree_value *value = gfp_tree_value_init(addr);

    list_add(&value->list, gfp_node->values);
}

/*
 * initalizes all of the zones that dont use a bitmap in the kernel binary
 */
void init_gfp_zones()
{
    struct bitmap_memory_zone *user_bitmap = gfp_user_zone.zone_private;

    user_bitmap->bitmap = malloc(user_bitmap->size / 8);

    struct gfp_tree_zone *tree_zone = malloc(sizeof(struct gfp_tree_zone));

#ifdef CONFIG_x86_64
    tree_zone->base = (void *)0xffffE00000000000;
#elif defined(CONFIG_x86_32)
    tree_zone->base = (void *)0xE0000000;
#endif

    tree_zone->head = bintree_head_init();

    map_page(get_free_phys_page(), tree_zone->base,
             PAGING_PRESENT | PAGING_WRITEABLE);
    struct gfp_tree_value *value =
        gfp_tree_value_init(tree_zone->base + (tree_zone->offset * 4096));
    tree_zone->offset += 4096;

    struct gfp_tree_node *node = gfp_tree_node_init(1);
    node->values = &value->list;

    bintree_add(tree_zone->head, &node->bintree);

    gfp_kernel_tree_zone.zone_private = tree_zone;

    gfp_zones_initialized = true;
}
