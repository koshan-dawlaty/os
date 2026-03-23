#include <extension.h>
#include <lib/log.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __is_libk
#include <serial.h>
#endif

#define LOG_SERIAL_PORT 0x3F8

static inline void log_serial_writechar(char c)
{
#ifdef __is_libk
    serial_write(LOG_SERIAL_PORT, c);
#else
    write(3, &c, 1);
#endif
}

static inline void log_serial_writestring(const char *s, size_t len)
{
#ifdef __is_libk
    serial_writestring(s, len, LOG_SERIAL_PORT);
#else
    write(3, s, len);
#endif
}

static inline void log_writechar(char c)
{
    log_serial_writechar(c);
}

static inline void log_writechars(const char *str, size_t len)
{
    log_serial_writestring(str, len);
}

static inline void log_writestring(const char *str)
{
    log_serial_writestring(str, strlen(str));
}

void vlog_level(struct log_level level, const char *str, va_list parameters)
{
    if (level.priority > LOG_PRIORITY_THRESHOLD.priority)
    {
        return;
    }

    char *s = vrprintf(str, parameters);

    log_writechar('[');
    log_writestring(level.str);
    log_writechar(']');
    log_writechar(' ');
    log_writestring(s);
    log_writechar('\n');

    free(s);
}
