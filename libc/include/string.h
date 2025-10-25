#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

size_t strlen(char *str);

void *memcpy(void *dest, void *src, size_t size);
void *memset(void *dest, int c, size_t n);

char *strdup(char *str);

// TODO: does not return which string is alphabetically greater
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t len);
int strcat(char *dest, char *src);

#endif
