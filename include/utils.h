#ifndef GIT_CHAT_UTILS_H
#define GIT_CHAT_UTILS_H

#include <sys/types.h>

#include "logging.h"

#define NORETURN __attribute__((noreturn))

/**
 * Frequently used error/information messages should be defined here.
 * */
#define MEM_ALLOC_FAILED "unable to allocate memory"
#define FILE_OPEN_FAILED "failed to open file '%s'"
#define FILE_WRITE_FAILED "failed to write to file '%s'"

/**
 * Frequently used macro constants should be defined here.
 * */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/**
 * Simple assertion function. Invoking this function will print a message to
 * stderr, and exit with status EXIT_FAILURE.
 *
 * This function is primarily used to catch fail fast when an undefined state
 * is encountered.
 *
 * If errno is set, an appropriate message is also printed to stderr.
 * */
NORETURN void BUG(const char *fmt, ...);

/**
 * Function used to terminate the application if an unexpected error occurs.
 * This function should be used in situations where the application encountered
 * an error that it cannot recover from, such as a NULL pointer returned from
 * a malloc() call.
 *
 * If errno is set, an appropriate message is also printed to stderr.
 *
 * The message is printed to stderr, and the application exits with status
 * EXIT_FAILURE.
 * */
NORETURN void FATAL(const char *fmt, ...);

/**
 * Function used to terminate the application if the user attempted to do
 * something that was unexpected.
 *
 * The message is printed to stderr, and the application exits with status
 * EXIT_FAILURE.
 *
 * If errno is set, an appropriate message is also printed to stderr.
 * */
NORETURN void DIE(const char *fmt, ...);

/**
 * Function used to warn the user of exceptional situations, but not fatal to
 * the application.
 *
 * The message is printed to stderr.
 * */
void WARN(const char *fmt, ...);

/**
 * Function used to inform the user.
 *
 * The message is printed to stderr.
 * */
void INFO(const char *fmt, ...);

/**
 * Configure which routine should be invoked to exit the current process.
 *
 * By default, exit(int) is used as the exit routine. However, this cannot be
 * used in certain situations, such as within a fork()'ed process. This
 * provides an interface to modify this behavior.
 * */
void set_exit_routine(NORETURN void (*new_exit_routine)(int status));

/**
 * A self-recovering wrapper for read(). If EINTR or EAGAIN is encountered,
 * retries read().
 * */
ssize_t xread(int fd, void *buf, size_t len);

/**
 * A self-recovering wrapper for write(). If EINTR or EAGAIN is encountered,
 * retries write().
 * */
ssize_t xwrite(int fd, const void *buf, size_t len);

#endif //GIT_CHAT_UTILS_H
