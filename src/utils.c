#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "utils.h"

static void print_message(FILE *output_stream, const char *prefix,
		const char *fmt, va_list varargs);

NORETURN void BUG(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "BUG: ", fmt, varargs);
	va_end(varargs);

	exit(EXIT_FAILURE);
}

NORETURN void FATAL(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "Fatal Error: ", fmt, varargs);
	va_end(varargs);

	if(errno > 0)
		fprintf(stderr, "%s\n", strerror(errno));

	exit(EXIT_FAILURE);
}

NORETURN void DIE(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	vfprintf(stderr, fmt, varargs);
	fprintf(stderr, "\n");
	va_end(varargs);

	exit(EXIT_FAILURE);
}

void WARN(const char *fmt, ...)
{
	va_list varargs;

	va_start(varargs, fmt);
	print_message(stderr, "Warning: ", fmt, varargs);
	va_end(varargs);
}

static void print_message(FILE *output_stream, const char *prefix,
		const char *fmt, va_list varargs)
{
	fprintf(output_stream, "%s", prefix);
	vfprintf(output_stream, fmt, varargs);
	fprintf(output_stream, "\n");
}