#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "utils.h"

static void print_message(FILE *output_stream, const char *prefix,
		const char *fmt, va_list varargs);
static NORETURN void default_exit_routine(int status);
static NORETURN void (*exit_routine)(int) = default_exit_routine;

NORETURN void BUG(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "BUG: ", fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

NORETURN void FATAL(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "Fatal Error: ", fmt, varargs);
	va_end(varargs);

	exit_routine(EXIT_FAILURE);
}

NORETURN void DIE(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	vfprintf(stderr, fmt, varargs);
	fprintf(stderr, "\n");
	va_end(varargs);

	if (errno > 0)
		fprintf(stderr, "%s\n", strerror(errno));

	exit_routine(EXIT_FAILURE);
}

void WARN(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "Warn: ", fmt, varargs);
	va_end(varargs);
}

static void print_message(FILE *output_stream, const char *prefix,
		const char *fmt, va_list varargs)
{
	fprintf(output_stream, "%s", prefix);
	vfprintf(output_stream, fmt, varargs);
	fprintf(output_stream, "\n");

	if (errno > 0)
		fprintf(stderr, "%s\n", strerror(errno));
}

void set_exit_routine(NORETURN void (*new_exit_routine)(int))
{
	exit_routine = new_exit_routine;
}

static NORETURN void default_exit_routine(int status)
{
	exit(status);
}
