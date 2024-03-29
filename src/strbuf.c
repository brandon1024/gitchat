#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "strbuf.h"
#include "utils.h"

#define BUFF_SLOP 64

void strbuf_init(struct strbuf *buff)
{
	buff->alloc = BUFF_SLOP;
	buff->len = 0;
	buff->buff = (char *)calloc(buff->alloc, sizeof(char));
	if (buff->buff == NULL)
		FATAL(MEM_ALLOC_FAILED);
}

void strbuf_release(struct strbuf *buff)
{
	free(buff->buff);
	buff->buff = NULL;
	buff->alloc = 0;
	buff->len = 0;
}

void strbuf_grow(struct strbuf *buff, size_t size)
{
	if ((size - 1) < buff->alloc)
		return;

	buff->alloc = size;
	buff->buff = (char *)realloc(buff->buff, buff->alloc * sizeof(char));
	if (!buff->buff)
		FATAL(MEM_ALLOC_FAILED);
}

void strbuf_attach(struct strbuf *buff, const char *str, size_t buffer_len)
{
	char *eos = memchr(str, 0, buffer_len);
	size_t str_len = (eos == NULL) ? buffer_len : (size_t)(eos - str);

	if ((buff->len + str_len + 1) >= buff->alloc)
		strbuf_grow(buff, buff->alloc + buffer_len + BUFF_SLOP);

	strncpy(buff->buff + buff->len, str, str_len);
	buff->buff[buff->len + str_len] = 0;

	buff->len += str_len;
}

void strbuf_attach_str(struct strbuf *buff, const char *str)
{
	strbuf_attach(buff, str, strlen(str));
}

void strbuf_attach_chr(struct strbuf *buff, char chr)
{
	char cbuf[2] = {chr, 0};
	strbuf_attach(buff, cbuf, 2);
}

void strbuf_attach_fmt(struct strbuf *buff, const char *fmt, ...)
{
	va_list varargs;
	va_start(varargs, fmt);

	strbuf_attach_vfmt(buff, fmt, varargs);

	va_end(varargs);
}

void strbuf_attach_vfmt(struct strbuf *buff, const char *fmt, va_list varargs)
{
	va_list varargs_cpy;
	va_copy(varargs_cpy, varargs);

	// calculate required buffer size
	ssize_t len = vsnprintf(NULL, 0, fmt, varargs_cpy);
	if (len < 0)
		FATAL("unexpected error from vsnprintf()");

	va_end(varargs_cpy);

	strbuf_grow(buff, buff->alloc + len + 1);
	len = vsnprintf(buff->buff + buff->len, len + 1, fmt, varargs);
	if (len < 0)
		FATAL("unexpected error from vsnprintf()");

	buff->len += len;
}

void strbuf_attach_fd(struct strbuf *buff, int fd)
{
	char temp_buffer[1024];
	ssize_t bytes_read;
	while ((bytes_read = xread(fd, temp_buffer, 1024)) > 0)
		strbuf_attach(buff, temp_buffer, bytes_read);

	if (bytes_read < 0)
		FATAL("pipe read failure");
}

int strbuf_trim(struct strbuf *buff)
{
	int chars_trimmed = 0;
	char *leading = buff->buff;
	char *index = buff->buff;

	//move to first non-space character
	while (leading < (buff->buff + buff->alloc) && (isspace(*leading) || isblank(*leading))) {
		leading++;
		chars_trimmed++;
	}

	//if string is entirely whitespace, simply null every character
	if (leading >= (buff->buff + buff->alloc) || !*leading) {
		while (index < leading)
			*(index++) = 0;

		buff->len -= chars_trimmed;
		return chars_trimmed;
	}

	if (leading != index) {
		//while leading is not null, copy leading over to index
		while (leading < (buff->buff + buff->alloc) && *leading) {
			*index = *leading;
			index++;
			leading++;
		}

		//be sure to copy the null byte
		*index = 0;
	} else {
		index = buff->buff + buff->len;
	}

	//when buffer is shifted, work backwards checking for whitespace, setting to nulls
	while (--index, isspace(*index) || isblank(*index)) {
		*index = 0;
		chars_trimmed++;
	}

	buff->len -= chars_trimmed;
	return chars_trimmed;
}

void strbuf_remove(struct strbuf *sb, size_t pos, size_t len)
{
	if (pos >= sb->len)
		return;
	if (!len)
		return;

	if ((sb->buff + pos + len) > (sb->buff + sb->len)) {
		// if removing all bytes after pos, just null buff[pos] and set length
		sb->len = pos;
	} else {
		// if removing bytes somewhere within buff, truncate bytes with memmove
		// and update length
		size_t partition_len = sb->buff + sb->len - (sb->buff + pos + len);
		memmove(sb->buff + pos, sb->buff + pos + len, partition_len);
		sb->len = pos + partition_len;
	}

	sb->buff[sb->len] = 0;
}

char *strbuf_detach(struct strbuf *buff)
{
	char *detached_buffer = buff->buff;
	buff->buff = NULL;
	strbuf_release(buff);

	return detached_buffer;
}

int strbuf_split(const struct strbuf *buff, const char *delim, struct str_array *result)
{
	if (!delim || !strlen(delim)) {
		str_array_push(result, buff->buff, NULL);
		return 1;
	}

	int inserted = 0;
	size_t delim_len = strlen(delim);
	char *begin = buff->buff;
	char *delim_ptr = NULL;

	do {
		delim_ptr = strstr(begin, delim);
		if (!delim_ptr)
			delim_ptr = strchr(begin, 0);
		if (!delim_len)
			BUG("strchr() could not find the null byte");

		char *substring = (char *)calloc((delim_ptr - begin) + 1, sizeof(char));
		if (!substring)
			FATAL(MEM_ALLOC_FAILED);

		strncpy(substring, begin, (delim_ptr - begin));
		str_array_insert_nodup(result, substring, result->len);

		begin = delim_ptr + delim_len;
		inserted++;
	} while(delim_ptr && *delim_ptr);

	return inserted;
}

void strbuf_clear(struct strbuf *buff)
{
	buff->len = 0;
	buff->buff[0] = 0;
}
