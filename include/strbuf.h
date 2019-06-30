#ifndef GIT_CHAT_STRBUF_H
#define GIT_CHAT_STRBUF_H

#include <stdlib.h>

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
void strbuf_attach(struct strbuf *buff, char *str, size_t buffer_len);

/**
 * Attach a null-terminated string to the strbuf. Similar to strbuf_attach(), except
 * uses strlen() to determine the buffer_len.
 * */
void strbuf_attach_str(struct strbuf *buff, char *str);

/**
 * Attach a single character to the strbuf.
 * */
void strbuf_attach_chr(struct strbuf *buff, char ch);

/**
 * Trim leading and trailing whitespace from an strbuf, returning the number of
 * characters removed from the buffer.
 *
 * The underlying buffer is not resized, rather characters are just shifted such
 * that no leading or trailing whitespace exists.
 * */
int strbuf_trim(struct strbuf *buff);

/**
 * Detach the string from the strbuf. The strbuf is released and must be
 * reinitialized for reuse.
 * */
char *strbuf_detach(struct strbuf *buff);

#endif //GIT_CHAT_STRBUF_H
