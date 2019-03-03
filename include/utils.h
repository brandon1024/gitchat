#ifndef GIT_CHAT_UTILS_H
#define GIT_CHAT_UTILS_H

#define NORETURN

/**
 * Simple assertion function. Invoking this function will cause the application
 * to print a message to stderr, and exit with status EXIT_FAILURE.
 *
 * This function is primarily used to catch fail fast when an undefined state
 * is encountered.
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
 * */
NORETURN void DIE(const char *fmt, ...);

/**
 * Function used to warn the user of some information that might be concerning.
 *
 * The message is printed to stderr.
 * */
void WARN(const char *fmt, ...);

#endif //GIT_CHAT_UTILS_H