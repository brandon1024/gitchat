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
NORETURN void BUG(const char *msg, ...);

#endif //GIT_CHAT_UTILS_H
