#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "strbuf.h"
#include "utils.h"

#define BUFF_SLOP 64

void strbuf_init(struct strbuf *buff)
{
	buff->alloc = BUFF_SLOP;
	buff->len = 0;
	buff->buff = (char *)calloc(buff->alloc, sizeof(char));
	if(buff->buff == NULL)
		FATAL("Unable to allocate memory.");
}

void strbuf_release(struct strbuf *buff)
{
	free(buff->buff);
	buff->buff = NULL;
	buff->alloc = 0;
	buff->len = 0;
}

void strbuf_attach(struct strbuf *buff, char *str, size_t buffer_len)
{
	char *eos = memchr(str, 0, buffer_len);
	size_t str_len = (eos == NULL) ? buffer_len : eos - str;

	if((buff->len + str_len + 1) >= buff->alloc) {
		buff->alloc += buffer_len + BUFF_SLOP;
		buff->buff = (char *)realloc(buff->buff, buff->alloc * sizeof(char));
		if(buff->buff == NULL)
			FATAL("Unable to allocate memory.");
	}

	strncpy(buff->buff + buff->len, str, str_len);
	buff->buff[buff->len + str_len] = 0;

	buff->len += str_len;
}

char *strbuf_detach(struct strbuf *buff)
{
	char *detached_buffer = buff->buff;
	strbuf_init(buff);

	return detached_buffer;
}