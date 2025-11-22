#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>
#include <stddef.h>

typedef struct
{
    int fd;
} FILE;

FILE *fopen(const char *path, const char *flags);
size_t fread(void *restrict, size_t, size_t, FILE *restrict);

int putchar(int ic);
int puts(char *c);

/* int printf(const char *format, ...) __attribute__((format(printf, 1, 2))); */
int printf(const char *format, ...);
int vprintf(const char *format, va_list);
char getchar();

#endif
