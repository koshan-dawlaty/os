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

struct log_level LOG_DEBUG = LOG_LEVEL_INIT("DEBUG", 3);
struct log_level LOG_INFO = LOG_LEVEL_INIT("INFO", 3);
struct log_level LOG_WARN = LOG_LEVEL_INIT("WARN", 3);
struct log_level LOG_ERROR = LOG_LEVEL_INIT("ERROR", 3);

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

void log_write_level(struct log_level level)
{
    log_writechar('[');
    log_writestring(level.str);
    log_writechar(']');
    log_writechar(' ');
}

size_t log_level_get_len(struct log_level level)
{
    return strlen(level.str) +
           3; /* TODO: the constant offset here could cause problems, maybe
   make the things printed with the log level constant */
}

void log_display_char(char c, struct log_level level)
{
    if (c == '\n')
    {
        log_writechar('\n');
        for (size_t i = 0; i < log_level_get_len(level); i++)
        {
            log_writechar(' ');
        }
    }
    else
    {
        log_writechar(c);
    }
}

void log_multiline_start(struct log_level *level)
{
#ifdef SHOULD_LOG
    if (level->multiline_count == 0)
    {
        log_write_level(*level);
        level->multiline_count++;
    }
#endif
}

void log_multiline_end(struct log_level *level)
{
#ifdef SHOULD_LOG
    if (level->multiline_count < 1)
    {
        level->multiline_count--;
        log_writechar('\n');
    }
#endif
}

void vlog_level(struct log_level level, const char *str, va_list parameters)
{
    if (level.priority > LOG_PRIORITY_THRESHOLD.priority)
    {
        return;
    }

    char *s = vrprintf(str, parameters);

    if (level.multiline_count == 0)
    {
        log_write_level(level);
    }

    char *s2 = s;
    while (*s2)
    {
        log_display_char(*s2, level);
        s2++;
    }
    if (level.multiline_count == 0)
    {
        log_writechar('\n');
    }
}
