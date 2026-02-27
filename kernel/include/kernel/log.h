#ifndef KERNEL_LOG_H
#define KERNEL_LOG_H

#include <stdarg.h>
struct log_level
{
    const char *str;
};

void vlog_level(struct log_level level, const char *str, va_list parameters);
static inline void log_level(struct log_level level, const char *str, ...)
{
    va_list parameters;
    va_start(parameters, str);

    vlog_level(level, str, parameters);

    va_end(parameters);
}

#define LOG_LEVEL_INIT(_str) ((struct log_level){.str = _str})

#define LOG_DEBUG LOG_LEVEL_INIT("DEBUG")
#define LOG_INFO LOG_LEVEL_INIT("INFO")
#define LOG_WARN LOG_LEVEL_INIT("WARN")
#define LOG_ERROR LOG_LEVEL_INIT("ERROR")
#define LOG_DEFAULT LOG_DEBUG

#define log(str, ...) log_level(LOG_DEFAULT, str, __VA_ARGS__)
#define logd(str, ...) log_level(LOG_DEBUG, str, __VA_ARGS__)
#define logi(str, ...) log_level(LOG_INFO, str, __VA_ARGS__)
#define logw(str, ...) log_level(LOG_WARN, str, __VA_ARGS__)
#define loge(str, ...) log_level(LOG_ERROR, str, __VA_ARGS__)

#endif
