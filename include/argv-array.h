#ifndef GIT_CHAT_ARGV_ARRAY_H
#define GIT_CHAT_ARGV_ARRAY_H

struct argv_array {
    char **argv;
    size_t argc;
    size_t alloc;
};

/**
 * Initialize an argv_array.
 * */
void argv_array_init(struct argv_array *argv_a);

/**
 * Release resources from an argv_array. The argv_array will return to its initial state.
 * */
void argv_array_release(struct argv_array *argv_a);

/**
 * Push one or more strings (const char *) to the argv_array.
 *
 * Each string is duplicated, and a pointer to it is stored under argv_a->argv at the next available
 * index in the buffer. If no space remains, the buffer is reallocated by a constant factor.
 *
 * The last argument MUST be NULL, to indicate the end of arguments.
 *
 * This function returns 1 on error, and zero otherwise.
 * */
int argv_array_push(struct argv_array *argv_a, ...);

/**
 * Pop the last value from the argv_array. The string memory is released, and argv_a->argc is updated
 * accordingly. argv_a->argv is not reallocated.
 * */
char *argv_array_pop(struct argv_array *argv_a);

/**
 * Detach from the argv_array the array of strings. The argv_array is reset to its initial state.
 *
 * The array of strings will need to be free()d manually.
 * */
char **argv_array_detach(struct argv_array *argv_a, size_t *len);

/**
 * Combine the arguments into a single newly allocated string. Unlike argv_array_detach, the argv_array
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
 * */
char *argv_array_collapse(struct argv_array *argv_a);

#endif //GIT_CHAT_ARGV_ARRAY_H
