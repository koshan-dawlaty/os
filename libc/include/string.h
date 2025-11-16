#ifndef _STRING_H
#define _STRING_H

#include <stddef.h>

size_t strlen(const char *str);

void *memcpy(void *dest, const void *src, size_t size);
void *memset(void *dest, int c, size_t n);

char *strdup(char *str);

/* TODO: does not return which string is alphabetically greater */
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t len);
char *strcat(char *dest, const char *src);
char *strtok(char *s, char *sep);
char *strtok_r(char *s, char *sep, char **state);

/* errno.c */
const char *strerror(int errnum);

#endif
