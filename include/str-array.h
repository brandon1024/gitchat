#ifndef GIT_CHAT_STR_ARRAY_H
#define GIT_CHAT_STR_ARRAY_H

#include <stdlib.h>
#include <stdarg.h>

/**
 * str-array api
 *
 * The str-array api is used to abstract the process of using dynamically
 * allocated arrays of strings. This api is general purpose.
 *
 * Data Structure:
 * struct str_array
 * - char **strings: array of pointers to strings.
 * - size_t len: number of strings stored under `strings`.
 * - size_t alloc: allocated size of the `strings` array.
 * */

struct str_array {
	char **strings;
	size_t len;
	size_t alloc;
};


/**
 * Initialize an str_array.
 * */
void str_array_init(struct str_array *str_a);

/**
 * Resize the str_array to fit, at least, the given size. If `size` is smaller
 * than the current size of the array, the array is left untouched.
 * */
void str_array_grow(struct str_array *str_a, size_t size);

/**
 * Release resources from an str_array. The str_array will return to its initial
 * state.
 *
 * Note: Care must be taken with strings that were inserted into the str_array
 * using the *_nodup() family of functions. If a string was inserted using one
 * of the nodup() functions, and str_array_release() is invoked, then some strings
 * managed by the caller may be free()d, which can result in undesired behavior.
 * */
void str_array_release(struct str_array *str_a);

/**
 * Retrieve a string from the str_array.
 *
 * Returns a pointer to the string, or NULL if pos >= str_a->len.
 * */
char *str_array_get(struct str_array *str_a, size_t pos);

/**
 * Replace a string in the str_array with a new string. The string is duplicated
 * before being inserted into the array.
 *
 * `str` must be a null-terminated string.
 *
 * Returns zero if successful, and non-zero if no element with index pos exists.
 * */
int str_array_set(struct str_array *str_a, char *str, size_t pos);

/**
 * Replace a string in the str_array with a new string. The string is not
 * duplicated, and str will be inserted directly into the array.
 *
 * `str` must be a null-terminated string.
 *
 * WARNING: calling str_array_release() on this str_array will free the string
 * being inserted. This might cause undesired behavior. It is recommended that
 * this function only be used to insert NULL pointers into the str_array.
 *
 * Returns zero if successful, and non-zero if no element with index pos exists.
 * */
int str_array_set_nodup(struct str_array *str_a, char *str, size_t pos);

/**
 * Push one or more strings to the end of the str_array.
 *
 * Each string is duplicated, and a pointer to it is stored under argv_a->argv
 * at the next available index in the buffer. If no space remains, the buffer is
 * reallocated by a constant factor.
 *
 * Each string must be null-terminated.
 *
 * The last argument MUST be NULL, to indicate the end of arguments.
 *
 * This function returns the number of items pushed to the str_array.
 * */
__attribute__ ((sentinel))
int str_array_push(struct str_array *str_a, ...);

/**
 * Identical to str_array_push, but accepts a va_list rather than variadic
 * arguments.
 * */
int str_array_vpush(struct str_array *str_a, va_list args);

/**
 * Insert a string into a given position in the str_array. The string is
 * duplicated before being inserted into the array.
 *
 * The string at position pos, and all subsequent strings in the array, are
 * shifted to the right to allow the new string to be inserted.
 *
 * If pos is greater than the size of the array, the new string will be inserted
 * at the end of the array.
 *
 * `str` must be a null-terminated string.
 *
 * This function returns the index of the location in which the string was
 * inserted.
 * */
size_t str_array_insert(struct str_array *str_a, char *str, size_t pos);

/**
 * Insert a string into a given position in the str_array. The string is not
 * duplicated before being inserted into the array.
 *
 * The string at position pos, and all subsequent strings in the array, are
 * shifted to the right to allow the new string to be inserted.
 *
 * If pos is greater than the size of the array, the new string will be inserted
 * at the end of the array.
 *
 * `str` must be a null-terminated string.
 *
 * WARNING: calling str_array_release() on this str_array will free the string
 * being inserted. This might cause undesired behavior. It is recommended that
 * this function only be used to insert NULL pointers into the str_array.
 *
 * This function returns the index of the location in which the string was
 * inserted.
 * */
size_t str_array_insert_nodup(struct str_array *str_a, char *str, size_t pos);

/**
 * Sort all entries in the str_array in 'strcmp()' order.
 * */
void str_array_sort(struct str_array *str_a);

/**
 * Remove a string from a given position in the str_array.
 *
 * All subsequent strings in the array are shifted to the left to fill the gap.
 *
 * The string removed from the array is not free()d, and must be handled by the
 * caller. If no string exists at the given position, a NULL pointer is returned.
 * */
char *str_array_remove(struct str_array *str_a, size_t pos);

/**
 * Remove all strings for the str_array.
 *
 * All strings are free()d, but the internal array is not reallocated and is not
 * free()d.
 * */
void str_array_clear(struct str_array *str_a);

/**
 * Detach from the str_array the array of strings. The str_array is reset to
 * its initial state.
 *
 * The array of strings will need to be free()d manually.
 * */
char **str_array_detach(struct str_array *str_a, size_t *len);

#endif //GIT_CHAT_STR_ARRAY_H
