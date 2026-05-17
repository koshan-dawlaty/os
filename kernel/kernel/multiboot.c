#include "multiboot.h"
#include "lib/log.h"
#include <kernel/graphics.h>
#include <kernel/multiboot.h>
#include <kernel/paging.h>
#include <kernel/types.h>
#include <memory/get_free_pages.h>
#include <memory/paging_flags.h>
#include <panic.h>
#include <stdalign.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_MEMORY_REGIONS_MAX 100
struct memory_region memory_regions[DEFAULT_MEMORY_REGIONS_MAX] = {0};
volatile size_t default_memory_regions_len = 0;
/* #define KERNEL_VIRTUAL_OFFSET 0xffffffff80100000 */

#ifdef CONFIG_x86_64
#define KERNEL_VIRTUAL_OFFSET 0xffffffff80000000
#elif defined(CONFIG_x86_32)
#define KERNEL_VIRTUAL_OFFSET 0xC0000000
#endif

#define CONVERT_PHYSADDR(addr)                                                 \
    (void *)(((void *)addr) - ((void *)KERNEL_VIRTUAL_OFFSET))

void __section(".multiboot.text") __no_ssp
    multiboot_test(struct multiboot_info *info, ptr magic)
{

    if (magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        panic_no_malloc("invalid grub magic number");
        return;
    }

    if (!(info->flags >> 6 & 0x1))
    {
        panic_no_malloc("invalid memory map given by grub");
        return;
    }
    multiboot_memory_map_t *mmmt;
    for (size_t i = 0; i < info->mmap_length;
         i += sizeof(multiboot_memory_map_t))
    {
        mmmt = (multiboot_memory_map_t *)(info->mmap_addr + i);

        /* printf("Start Addr: %x | Length: %x | Size: %x | Type: %d\n", */
        /*        mmmt->addr, mmmt->len, mmmt->size, mmmt->type); */

        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
            struct memory_region *memory_regions_ptr =
                CONVERT_PHYSADDR(&memory_regions);
            size_t *length_ptr = CONVERT_PHYSADDR(&default_memory_regions_len);
            (*length_ptr)++;

            memory_regions_ptr[(*length_ptr) - 1] = (struct memory_region){
                .start = (void *)((ptr)mmmt->addr), .len = mmmt->len};
        }
    }
}

/* we can assume these are next to each other, struct then magic */
extern u32 multiboot_struct_physical;
extern u32 multiboot_magic_physical;

/*
 * @info needs to be mapped
 */
void multiboot_parse_memory_map(struct multiboot_info *info)
{
    void *memory_map = get_free_pages_unmmaped(
        ((info->mmap_length * sizeof(multiboot_memory_map_t)) / 4096) + 1,
        &gfp_kernel_zone);
    map_page((void *)(ptr)info->mmap_addr, memory_map,
             PAGING_PRESENT | PAGING_WRITEABLE);

    for (size_t i = 0; i < info->mmap_length;
         i += sizeof(multiboot_memory_map_t))
    {
        multiboot_memory_map_t *mmmt =
            (multiboot_memory_map_t *)(memory_map + i);

        /* printf("Start Addr: %p | Length: %p | Size: %p | Type: %d\n", */
        /*        mmmt->addr, mmmt->len, mmmt->size, mmmt->type); */
        if (mmmt->type == MULTIBOOT_MEMORY_AVAILABLE)
        {
        }
    }
}

struct bootloader_module multiboot_load_module(multiboot_module_t *modules,
                                               size_t index)
{

    logd("%d", sizeof(multiboot_module_t));
    multiboot_module_t *mod = &modules[index];
    size_t mod_size = mod->mod_end - mod->mod_start;
    size_t num_pages = (mod_size / PAGE_SIZE) + 1;
    logd("start:%p end:%p num_pages:%p", mod->mod_start, mod->mod_end,
         num_pages);

    void *addr = get_free_pages_unmmaped(num_pages, &gfp_kernel_zone);
    for (size_t i = 0; i < num_pages; i++)
    {
        map_page(((void *)(ptr)mod->mod_start) + i * PAGE_SIZE,
                 addr + i * PAGE_SIZE, PAGING_PRESENT | PAGING_WRITEABLE);
    }

    return (struct bootloader_module){addr + mod->mod_start % PAGE_SIZE,
                                      mod_size};
}

extern struct bootloader_module bootloader_symbol_table;

struct framebuffer *multiboot_init()
{

    void *virtual = get_free_pages_unmmaped(1, &gfp_kernel_zone);
    map_page(&multiboot_struct_physical, virtual,
             PAGING_PRESENT | PAGING_WRITEABLE);

    u32 multiboot_struct_addr = *(u32 *)(virtual);
    u32 multiboot_magic = *((u32 *)(virtual) + 1);

    /* printf("magic:%ux\n", multiboot_magic); */

    struct multiboot_info *info_physical = (void *)(ptr)multiboot_struct_addr;

    /* printf("info addr:%p\n", info_physical); */

    struct multiboot_info *info = get_free_pages_unmmaped(1, &gfp_kernel_zone);
    map_page((void *)(ptr)multiboot_struct_addr, info,
             PAGING_PRESENT | PAGING_WRITEABLE);

    logd("grub found %d", info->mods_count);
    if (info->flags & MULTIBOOT_INFO_MODS)
    {
#ifdef CONFIG_SYMBOL_TABLE
        multiboot_module_t *modules_physical = (void *)(ptr)info->mods_addr;
        size_t num_pages =
            ((info->mods_count * sizeof(multiboot_module_t)) / PAGE_SIZE) + 1;

        logd("physaddr:%p", modules_physical);

        size_t diff = info->mods_addr - align_long(info->mods_addr, PAGE_SIZE);
        logd("%p", diff);
        void *modules = get_free_pages_unmmaped(num_pages, &gfp_kernel_zone);

        for (size_t i = 0; i < num_pages; i++)
        {
            map_page(modules_physical + (i * PAGE_SIZE),
                     modules + (i * PAGE_SIZE),
                     PAGING_PRESENT | PAGING_WRITEABLE);
        }

        bootloader_symbol_table = multiboot_load_module(modules + diff, 0);
#endif
    }
    if ((info->flags >> 6 & 0x1))
    {
        /* multiboot_parse_memory_map(info); */
    }
    else
    {
        panic("invalid grub memory map");
    }
    /* printf("%d\n", offsetof(struct multiboot_info, framebuffer_height)); */

    if (info->flags & (1 << 12))
    {
        /* printf("good\n"); */
        /* printf("framebuffer addr: %p\n", info->framebuffer_addr); */
        /* printf("width x height: %u x %u\n", info->framebuffer_width, */
        /*        info->framebuffer_height); */
        /* printf("bpp: %u\n", info->framebuffer_bpp); */
    }
    else
    {
        return NULL;
    }
    if (info->framebuffer_type != 1)
    {

        return NULL;
    }

    /* printf("framebuffer:%p\n", info->framebuffer_addr); */

    /* size_t total_size = info->framebuffer_width * (info->framebuffer_bpp / 8)
     * * */
    /*                     info->framebuffer_height * info->framebuffer_pitch;
     */
    size_t total_size = info->framebuffer_pitch * info->framebuffer_height;
    size_t num_pages = (total_size / 4096) + 1;
    void *framebuffer = get_free_pages_unmmaped(num_pages, &gfp_kernel_zone);

    for (size_t i = 0; i < num_pages; i++)
    {
        map_page((void *)(ptr)info->framebuffer_addr + (i * PAGE_SIZE),
                 framebuffer + (i * PAGE_SIZE),
                 PAGING_PRESENT | PAGING_WRITEABLE);
    }

    /* memset(framebuffer, 'a', total_size) */

    /* printf("pitch:%p\n", info->framebuffer_pitch); */
    struct framebuffer *f = malloc(sizeof(struct framebuffer));
    f->buffer = framebuffer;
    f->depth = 24;
    f->height = info->framebuffer_height;
    f->width = info->framebuffer_width;
    f->pitch = info->framebuffer_pitch;
    f->pixel_width = (info->framebuffer_bpp / 8);

    /* for (size_t i = 0; i < 100; i++) */
    /* { */
    /*     framebuffer_put_pixel(f, 0xffffff, i, 0); */
    /*     framebuffer_put_pixel(f, 0x00ffff, i, f->height - 1); */
    /* } */

    return f;
    /* memset(framebuffer, 'a', 100); */
}
