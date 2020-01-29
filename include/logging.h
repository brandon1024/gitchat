#ifndef GIT_CHAT_LOGGING_H
#define GIT_CHAT_LOGGING_H

/**
 * Logging API
 *
 * Used primarily for development purposes, the logging API can be used to
 * print helpful debugging information to stdout and stderr.
 *
 * Log messages should never be displayed to the user, unless the user
 * explicitly wishes to see them by setting the `GIT_CHAT_LOG_LEVEL` environment
 * variable. As such, by default, the `NONE` log level is used.
 *
 * Log Levels:
 * 0	ALL
 * 1	TRACE
 * 2	DEBUG
 * 4	INFO
 * 8	WARN
 * 16	ERROR
 * 32	NONE
 * */

#ifdef RUNTIME_LOGGING

void log_trace(const char *file, int line_number, const char *fmt, ...);
void log_debug(const char *file, int line_number, const char *fmt, ...);
void log_info(const char *file, int line_number, const char* fmt, ...);
void log_warn(const char *file, int line_number, const char *fmt, ...);
void log_error(const char *file, int line_number, const char *fmt, ...);

/**
 * Log a message to stdout at the trace level. Arguments to this function are
 * passed directly to fprintf().
 * */
#define LOG_TRACE(...) log_trace(__FILE__, __LINE__, __VA_ARGS__)

/**
 * Log a message to stdout at the DEBUG level. Arguments to this function are
 * passed directly to fprintf().
 * */
#define LOG_DEBUG(...) log_debug(__FILE__, __LINE__, __VA_ARGS__)

/**
 * Log a message to stdout at the INFO level. Arguments to this function are
 * passed directly to fprintf().
 * */
#define LOG_INFO(...) log_info(__FILE__, __LINE__, __VA_ARGS__)

/**
 * Log a message to stderr at the WARN level. Arguments to this function are
 * passed directly to fprintf().
 * */
#define LOG_WARN(...) log_warn(__FILE__, __LINE__, __VA_ARGS__)

/**
 * Log a message to stderr at the ERROR level. Arguments to this function are
 * passed directly to fprintf().
 * */
#define LOG_ERROR(...) log_error(__FILE__, __LINE__, __VA_ARGS__)

#else
#define LOG_TRACE(...) (void)0
#define LOG_DEBUG(...) (void)0
#define LOG_INFO(...) (void)0
#define LOG_WARN(...) (void)0
#define LOG_ERROR(...) (void)0
#endif

#endif //GIT_CHAT_LOGGING_H
