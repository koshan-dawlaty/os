#ifndef OS_MULTIBOOT_H
#define OS_MULTIBOOT_H

#include <stddef.h>
struct memory_region
{
    void *start;
    size_t len;
};

extern struct memory_region memory_regions[];
extern volatile size_t default_memory_regions_len;

static inline size_t memory_regions_len()
{
    return default_memory_regions_len;
}

static inline struct memory_region memory_regions_get(size_t index)
{
    /* TODO: this is to prevent overwriting the kernel, hopefully there is
     * enough space in the region to skip the size of the kernel */
    if (memory_regions[index].start == (void *)0x100000)
    {
        struct memory_region out = memory_regions[index];
        out.start += 0x300000;
        return out;
    }
    return memory_regions[index];
}

struct framebuffer *multiboot_init();

#endif
