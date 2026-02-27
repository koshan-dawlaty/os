#include <extension.h>
#include <kernel/log.h>
#include <serial.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define LOG_SERIAL_PORT 0x3F8

void log_writechar(char c)
{
    serial_write(LOG_SERIAL_PORT, c);
}

void log_writechars(const char *str, size_t len)
{
    serial_writestring(str, len, LOG_SERIAL_PORT);
}

void log_writestring(const char *str)
{
    serial_writestring(str, strlen(str), LOG_SERIAL_PORT);
}

void vlog_level(struct log_level level, const char *str, va_list parameters)
{
    char *s = vrprintf(str, parameters);

    log_writechar('[');
    log_writestring(level.str);
    log_writechar(']');
    log_writechar(' ');
    log_writestring(s);
    log_writechar('\n');
}
