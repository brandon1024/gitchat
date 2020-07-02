#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"
#include "run-command.h"
#include "argv-array.h"

static void print_message(FILE *, const char *, const char *, va_list);
static NORETURN void default_exit_routine(int);
static NORETURN void (*exit_routine)(int) = default_exit_routine;

NORETURN void BUG(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "BUG", fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

NORETURN void FATAL(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "fatal", fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

NORETURN void DIE(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, NULL, fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

void WARN(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "warn", fmt, varargs);
	va_end(varargs);
}

static void print_message(FILE *output_stream, const char *prefix,
		const char *fmt, va_list varargs)
{
	struct strbuf message_buffer;
	strbuf_init(&message_buffer);

	strbuf_attach_vfmt(&message_buffer, fmt, varargs);

	if (prefix) {
		struct str_array lines;
		str_array_init(&lines);
		strbuf_split(&message_buffer, "\n", &lines);
		strbuf_clear(&message_buffer);

		for (size_t i = 0; i < lines.len; i++) {
			const char *line = str_array_get(&lines, i);
			strbuf_attach_fmt(&message_buffer, "%s: %s\n", prefix, line);
		}

		str_array_release(&lines);
	} else {
		strbuf_attach_fmt(&message_buffer, "\n");
	}

	fputs(message_buffer.buff, output_stream);

	if (errno > 0)
		fprintf(stderr, "%s\n", strerror(errno));

	strbuf_release(&message_buffer);
}

void set_exit_routine(NORETURN void (*new_exit_routine)(int))
{
	exit_routine = new_exit_routine;
}

static NORETURN void default_exit_routine(int status)
{
	exit(status);
}

ssize_t xread(int fd, void *buf, size_t len)
{
	int errsv = errno;

	ssize_t bytes_read = 0;
	while(1) {
		bytes_read = read(fd, buf, len);
		if ((bytes_read < 0) && (errno == EAGAIN || errno == EINTR)) {
			errno = errsv;
			continue;
		}

		break;
	}

	return bytes_read;
}

ssize_t xwrite(int fd, const void *buf, size_t len)
{
	int errsv = errno;

	ssize_t bytes_written = 0;
	while(1) {
		bytes_written = write(fd, buf, len);
		if ((bytes_written < 0) && (errno == EAGAIN || errno == EINTR)) {
			errno = errsv;
			continue;
		}

		break;
	}

	return bytes_written;
}
