#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>

int putchar(int ic);
int puts(char *c);

/* int printf(const char *format, ...) __attribute__((format(printf, 1, 2))); */
int printf(const char *format, ...);
int vprintf(const char *format, va_list);
char getchar();

#endif
