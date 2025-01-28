#ifndef STDIO_H
#define STDIO_H

#include <stdarg.h>
#include <stdint.h>

int putchar(int ic);
int puts(char *c);

int printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
int vprintf(const char *format, va_list);
int vsprintf(char *s, const char *format, va_list);

char getchar();

#endif
