#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

int atoi(char *in);

void *malloc(size_t size);
void free(void *ptr);
void *realloc(void *ptr, size_t new_size);
void *calloc(size_t nelem, size_t elsize);

void exit(int status);

void init_heap();

#endif
