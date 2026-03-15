#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <compiler.h>
#include <kernel/types.h>
#include <stdarg.h>
struct log_level
{
    const char *str;
    const u32 priority; /* 0 is lowest */
};

void vlog_level(struct log_level level, const char *str, va_list parameters);
static inline void log_level(struct log_level level, const char *str, ...)
{
    va_list parameters;
    va_start(parameters, str);

    vlog_level(level, str, parameters);

    va_end(parameters);
}

#define LOG_LEVEL_INIT(_str, p) ((struct log_level){.str = _str, .priority = p})

#define LOG_DEBUG LOG_LEVEL_INIT("DEBUG", 3)
#define LOG_INFO LOG_LEVEL_INIT("INFO", 2)
#define LOG_WARN LOG_LEVEL_INIT("WARN", 1)
#define LOG_ERROR LOG_LEVEL_INIT("ERROR", 0)
#define LOG_DEFAULT LOG_DEBUG

#define LOG_PRIORITY_THRESHOLD LOG_DEFAULT

#define log_macro(level, str, ...)                                             \
    log_level(level, str __VA_OPT__(, ) __VA_ARGS__)

#if defined(CONFIG_LOG) && !defined(TESTS)

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

#endif
