#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#if defined(CONFIG_LOG) && !defined(TESTS)
#define SHOULD_LOG
#endif

#include <compiler.h>
#include <math.h>
#include <stdarg.h>
#include <types.h>
struct log_level
{
    const char *str;
    const u32 priority; /* 0 is lowest */
    struct
    {
        bool is_multiline : 1;
        bool multiline_ : 1;
    };
};

void vlog_level(struct log_level level, const char *str, va_list parameters);
static inline void log_level(struct log_level level, const char *str, ...)
{
    va_list parameters;
    va_start(parameters, str);

    vlog_level(level, str, parameters);

    va_end(parameters);
}

#define LOG_LEVEL_INIT(_str, p)                                                \
    ((struct log_level){.str = _str, .priority = p, .is_multiline = false})

static struct log_level LOG_DEBUG = LOG_LEVEL_INIT("DEBUG", 3);
static struct log_level LOG_INFO = LOG_LEVEL_INIT("INFO", 3);
static struct log_level LOG_WARN = LOG_LEVEL_INIT("WARN", 3);
static struct log_level LOG_ERROR = LOG_LEVEL_INIT("ERROR", 3);
#define LOG_DEFAULT LOG_DEBUG

#define LOG_PRIORITY_THRESHOLD LOG_DEFAULT

#define log_macro(level, str, ...)                                             \
    log_level(level, str __VA_OPT__(, ) __VA_ARGS__)

#ifdef SHOULD_LOG

#define logd(str, ...) log_macro(LOG_DEBUG, str, __VA_ARGS__)
#define logi(str, ...) log_macro(LOG_INFO, str, __VA_ARGS__)
#define logw(str, ...) log_macro(LOG_WARN, str, __VA_ARGS__)
#define loge(str, ...) log_macro(LOG_ERROR, str, __VA_ARGS__)

#else

#define log(str, ...)
#define logd(str, ...)
#define logi(str, ...)
#define logw(str, ...)
#define loge(str, ...)

#endif

/* TODO: these should be moved to the CONFIG_LOG block, but build system bad */
void log_multiline_start(struct log_level *level);
void log_multiline_end(struct log_level *level);

#endif
