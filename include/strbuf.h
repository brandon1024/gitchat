#ifndef GIT_CHAT_STRBUF_H
#define GIT_CHAT_STRBUF_H

#include <stdlib.h>

#include "str-array.h"

/**
 * strbuf api
 *
 * The strbuf api is used as an alternative to dynamically allocating character
 * arrays. strbuf's are linearly resized when needed.
 *
 * Conventional string buffers can be easily attached (appended) to a strbuf, and
 * accessed using the strbuf->buff property. The internal buffer is guaranteed
 * to be null-terminated.
 * */

struct strbuf {
	char *buff;
	size_t len;
	size_t alloc;
};

/**
 * Initialize the strbuf. Enough space is allocated for an empty string, so that
 * calls to detach will always return a valid null-terminated string.
 * */
void strbuf_init(struct strbuf *buff);

/**
 * Release any resources under the strbuf. The strbuf MUST be reinitialized for
 * reuse.
 * */
void strbuf_release(struct strbuf *buff);

/**
 * Reallocate the strbuf to the given size. If the buffer is already larger than
 * size, the buffer is left as is.
 * */
void strbuf_grow(struct strbuf *buff, size_t size);

/**
 * Attach a string to the strbuf, up to buffer_len characters or until null byte
 * is encountered.
 * */
void strbuf_attach(struct strbuf *buff, const char *str, size_t buffer_len);

/**
 * Attach a null-terminated string to the strbuf. Similar to strbuf_attach(), except
 * uses strlen() to determine the buffer_len.
 * */
void strbuf_attach_str(struct strbuf *buff, const char *str);

/**
 * Attach a single character to the strbuf.
 * */
void strbuf_attach_chr(struct strbuf *buff, char ch);

/**
 * Attach a formatted string to the strbuf.
 *
 * The formatted string is constructed using snprintf(), potentially requiring
 * everal passes to allocate a buffer sufficiently large to hold the formatting
 * string.
 * */
void strbuf_attach_fmt(struct strbuf *buff, const char *fmt, ...);

/**
 * Trim leading and trailing whitespace from an strbuf, returning the number of
 * characters removed from the buffer.
 *
 * The underlying buffer is not resized, rather characters are just shifted such
 * that no leading or trailing whitespace exists. Note that this may not be
 * efficient with very large buffers.
 * */
int strbuf_trim(struct strbuf *buff);

/**
 * Detach the string from the strbuf. The strbuf is released and must be
 * reinitialized for reuse.
 * */
char *strbuf_detach(struct strbuf *buff);

/**
 * Split a strbuf into multiple strings on a given delimiter.
 *
 * The split strings are pushed into the given str_array. The given strbuf is not
 * modified.
 *
 * If the delimiter is NULL or the empty string, the entire strbuf is inserted into
 * the str_array. The delimiter must be a null-terminated string.
 *
 * Returns the number of entries added to str_array, or -1 if an error
 * occurred.
 * */
int strbuf_split(const struct strbuf *buff, const char *delim,
		struct str_array *result);

#endif //GIT_CHAT_STRBUF_H
