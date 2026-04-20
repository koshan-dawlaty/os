#ifndef PANIC_H
#define PANIC_H

#include <lib/assert.h>
#include <stdarg.h>

// void panic(const char *message, ...);
void vpanic(const char *message, va_list parameters);
void panic_no_malloc(const char *message);

#endif
