#include "psu_font.h"
#include <compiler.h>
#include <kernel/graphics.h>
#include <kernel/paging.h>
#include <kernel/syscall.h>
#include <kmalloc.h>
#include <lib/font.h>
#include <memory/get_free_pages.h>
#include <memory/paging_flags.h>
#include <stdio.h>
#include <syscall.h>
#include <types.h>

struct PSF_font default_font;
struct framebuffer *current_framebuffer;
void graphics_init(struct framebuffer *f)
{
    default_font = *font_parse((u8 *)_psfu_font);
    current_framebuffer = f;
}

/* TODO: should be inline */
struct framebuffer *get_current_framebuffer()
{
    return current_framebuffer;
}

struct framebuffer *framebuffer_alloc_user(struct framebuffer *buffer)
{
    void *physaddr = get_physaddr(buffer->buffer);
    size_t total_size = buffer->pitch * buffer->height;
    size_t num_pages = (total_size / 4096) + 1;
    /* TODO: this should probably not just map this with a bitmap allocator */
    void *user_buffer = get_free_pages_unmmaped(num_pages, &gfp_user_zone);

    for (size_t i = 0; i < num_pages; i++)
    {
        map_page((void *)(ptr)physaddr + (i * PAGE_SIZE),
                 user_buffer + (i * PAGE_SIZE),
                 PAGING_PRESENT | PAGING_USER | PAGING_WRITEABLE);
    }
    struct framebuffer *out = kmalloc(sizeof(struct framebuffer), KMALLOC_USER);
    out->buffer = user_buffer;
    out->depth = buffer->depth;
    out->height = buffer->height;
    out->width = buffer->width;
    out->pixel_width = buffer->pixel_width;
    out->pitch = buffer->pitch;

    return out;
}

void syscall_alloc_framebuffer(struct framebuffer *ptr)
{
}
