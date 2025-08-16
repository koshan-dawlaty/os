#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

int atoi(char *in);

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t new_size);

void exit(int status);

#endif
