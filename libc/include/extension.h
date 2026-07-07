#ifndef EXTENSION_H
#define EXTENSION_H
/*
 * everything in this file is not standard
 */

#include "sys/syscall.h"
#include "utils.h"
#include <stdarg.h>

/*
 * version of vsprintf that uses a heap pointer that it reallocs
 */
char *vrprintf(const char *format, va_list list);
char *rprintf(char *format, ...);

#endif
