#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <compiler.h>
#include <kernel/types.h>
#include <lib/framebuffer.h>
#include <stddef.h>

void framebuffer_draw_char(struct framebuffer *f, char c, size_t x, size_t y);

void graphics_init(struct framebuffer *f);

/*
 * returns a copy of @buffer that has a user accessible pointer to the buffer
 */
struct framebuffer *framebuffer_alloc_user(struct framebuffer *buffer);

struct framebuffer *get_current_framebuffer();

#endif
