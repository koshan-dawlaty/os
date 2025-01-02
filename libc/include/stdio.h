#ifndef STDIO_H
#define STDIO_H

int putchar(int ic);
int puts(char *c);

int printf(const char *format, ...) __attribute__((format(printf, 1, 2)));
char getchar();

#endif
