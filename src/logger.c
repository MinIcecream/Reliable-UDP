#include "logger.h"

#include <stdio.h>
#include <time.h>

static FILE *g_log_file = NULL;
static log_source_t g_source = LOG_SOURCE_SYSTEM;

static const char *source_to_string(log_source_t source) {
    switch (source) {
        case LOG_SOURCE_CLIENT:
            return "CLIENT";
        case LOG_SOURCE_SERVER:
            return "SERVER";
        case LOG_SOURCE_SYSTEM:
        default:
            return "SYSTEM";
    }
}

static const char *level_to_string(log_level_t level) {
    switch (level) {
        case LOG_LEVEL_DEBUG:
            return "DEBUG";
        case LOG_LEVEL_INFO:
            return "INFO";
        case LOG_LEVEL_WARN:
            return "WARN";
        case LOG_LEVEL_ERROR:
        default:
            return "ERROR";
    }
}

static void ensure_log_file(void) {
    if (g_log_file == NULL) {
        g_log_file = stderr;
    }
}

static void format_time(char *buffer, size_t buffer_len) {
    time_t now = time(NULL);
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(buffer, buffer_len, "%Y-%m-%d %H:%M:%S", &tm_info);
}

void log_init(log_source_t source, const char *path) {
    g_source = source;

    if (g_log_file != NULL && g_log_file != stderr) {
        fclose(g_log_file);
    }
    g_log_file = NULL;

    if (path != NULL && path[0] != '\0') {
        g_log_file = fopen(path, "a");
        if (g_log_file == NULL) {
            g_log_file = stderr;
        }
    } else {
        g_log_file = stderr;
    }
}

void log_set_source(log_source_t source) {
    g_source = source;
}

void log_close(void) {
    if (g_log_file != NULL && g_log_file != stderr) {
        fclose(g_log_file);
    }
    g_log_file = stderr;
}

static void log_message_internal(log_source_t source, log_level_t level, const char *fmt, va_list args) {
    char time_buf[32];
    ensure_log_file();
    format_time(time_buf, sizeof(time_buf));

    fprintf(g_log_file, "[%s] [%s] [%s] ", time_buf, source_to_string(source), level_to_string(level));
    vfprintf(g_log_file, fmt, args);
    fprintf(g_log_file, "\n");
    fflush(g_log_file);
}

void log_message(log_level_t level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message_internal(g_source, level, fmt, args);
    va_end(args);
}

void log_message_source(log_source_t source, log_level_t level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message_internal(source, level, fmt, args);
    va_end(args);
}
