#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "lib/font.h"
#include <compiler.h>
#include <lib/framebuffer.h>
#include <stddef.h>
#include <types.h>

void graphics_init(struct framebuffer *f);

/*
 * returns a copy of @buffer that has a user accessible pointer to the buffer
 */
struct framebuffer *framebuffer_alloc_user(struct framebuffer *buffer);

struct framebuffer *get_current_framebuffer();
extern struct PSF_font default_font;

#endif
