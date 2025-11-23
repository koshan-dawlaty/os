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
size_t fwrite(const void *restrict buf, size_t size, size_t number, FILE *f);
void rewind(FILE *f);

int putchar(int ic);
int puts(char *c);

/* int printf(const char *format, ...) __attribute__((format(printf, 1, 2))); */
int printf(const char *format, ...);
int vprintf(const char *format, va_list);
char getchar();

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#endif
