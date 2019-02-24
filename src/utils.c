#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "utils.h"

NORETURN void BUG(const char *msg, ...)
{
	va_list varargs;
	va_start(varargs, msg);

	fprintf(stderr, "BUG: ");
	vfprintf(stderr, msg, varargs);
	fprintf(stderr, "\n");
	va_end(varargs);

	exit(EXIT_FAILURE);
}
