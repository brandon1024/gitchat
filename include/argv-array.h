#ifndef GIT_CHAT_ARGV_ARRAY_H
#define GIT_CHAT_ARGV_ARRAY_H

#include <stdlib.h>
#include "str-array.h"

/**
 * argv-array api
 *
 * The argv-array api is mainly used to build a list of arguments which can be used
 * to assemble command arguments passed to a subprocess, or to manipulate
 * arguments passed to builtins.
 *
 * Since argv_array is just an str_array under the hood, it is possible to use
 * any str_array functions on an argv_array thanks to memory alignment.
 * */

struct argv_array {
	struct str_array arr;
};

/**
 * Initialize an argv_array.
 * */
void argv_array_init(struct argv_array *argv_a);

/**
 * Release resources from an argv_array. The argv_array will return to its
 * initial state.
 * */
void argv_array_release(struct argv_array *argv_a);

/**
 * Push one or more strings (const char *) to the argv_array.
 *
 * Each string is duplicated, and a pointer to it is stored under argv_a->argv
 * at the next available index in the buffer. If no space remains, the buffer is
 * reallocated by a constant factor.
 *
 * The last argument MUST be NULL, to indicate the end of arguments.
 *
 * This function returns the number of items pushed to the argv_array.
 * */
int argv_array_push(struct argv_array *argv_a, ...);

/**
 * Pop the last value from the argv_array. The popped string is returned, and
 * must be freed by the caller. argv_a->argv is not reallocated.
 *
 * If argv_a->argc is 0 (argv_array is empty), NULL is returned.
 * */
char *argv_array_pop(struct argv_array *argv_a);

/**
 * Prepend one or more strings (const char *) to the beginning of the argv_array.
 *
 * Similarly to argv_array_push, each string is duplicated and a pointer to it
 * is stored under argv_a->argv. Internally, the new strings to prepend are
 * pushed to a new argv_array, and then the arg_a values are then pushed on top.
 *
 * The last argument must be NULL, to indicate the end of arguments.
 *
 * This function returns the number of items prepended to the argv_array.
 * */
int argv_array_prepend(struct argv_array *argv_a, ...);

/**
 * Detach from the argv_array the array of strings. The argv_array is reset to
 * its initial state.
 *
 * The array of strings will need to be free()d manually.
 * */
char **argv_array_detach(struct argv_array *argv_a, size_t *len);

/**
 * Combine the arguments into a single newly allocated string, where each string
 * is delimited by a single space. Unlike argv_array_detach, the argv_array
 * remains untouched, and still needs to be freed using argv_array_release().
 *
 * The new string will need to be free()d manually.
 *
 * Example:
 * argv_array_push(&argv_a, "grep", "-i", query, filename, NULL);
 * char *result = argv_array_collapse(&argv_a);
 * printf("%s", result);
 * argv_array_release(&argv_a);
 * free(result);
 *
 * > "grep -i yourquery filename"
 *
 * If the argv_array is empty, returns NULL.
 * */
char *argv_array_collapse(struct argv_array *argv_a);

/**
 * Similar to argv_array_collapse(), but delimit each string in the argv_array
 * with the specified delimiter.
 *
 * Unlike argv_array_detach, the argv_array remains untouched, and still needs
 * to be freed using argv_array_release().
 *
 * The new string will need to be free()d manually.
 *
 * If the argv_array is empty, returns NULL.
 * */
char *argv_array_collapse_delim(struct argv_array *argv_a, const char *delim);

#endif //GIT_CHAT_ARGV_ARRAY_H
