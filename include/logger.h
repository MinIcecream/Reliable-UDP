#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

typedef enum {
    LOG_SOURCE_SYSTEM,
    LOG_SOURCE_CLIENT,
    LOG_SOURCE_SERVER
} log_source_t;

typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} log_level_t;

void log_init(log_source_t source, const char *path);
void log_set_source(log_source_t source);
void log_close(void);

void log_message(log_level_t level, const char *fmt, ...);
void log_message_source(log_source_t source, log_level_t level, const char *fmt, ...);

#define LOG_DEBUG(...) log_message(LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) log_message(LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_WARN(...) log_message(LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, __VA_ARGS__)

#define LOG_SYS_DEBUG(...) log_message_source(LOG_SOURCE_SYSTEM, LOG_LEVEL_DEBUG, __VA_ARGS__)
#define LOG_SYS_INFO(...) log_message_source(LOG_SOURCE_SYSTEM, LOG_LEVEL_INFO, __VA_ARGS__)
#define LOG_SYS_WARN(...) log_message_source(LOG_SOURCE_SYSTEM, LOG_LEVEL_WARN, __VA_ARGS__)
#define LOG_SYS_ERROR(...) log_message_source(LOG_SOURCE_SYSTEM, LOG_LEVEL_ERROR, __VA_ARGS__)

#endif // LOGGER_H
