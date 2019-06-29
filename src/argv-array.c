#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "argv-array.h"
#include "utils.h"

void argv_array_init(struct argv_array *argv_a)
{
	str_array_init(&argv_a->arr);
}

void argv_array_release(struct argv_array *argv_a)
{
	str_array_release(&argv_a->arr);
}

int argv_array_push(struct argv_array *argv_a, ...)
{
	va_list ap;
	va_start(ap, argv_a);
	int new_args = str_array_vpush(&argv_a->arr, ap);
	va_end(ap);

	return new_args;
}

char *argv_array_pop(struct argv_array *argv_a)
{
	return str_array_remove(&argv_a->arr, argv_a->arr.len - 1);
}

int argv_array_prepend(struct argv_array *argv_a, ...)
{
	va_list ap;
	va_start(ap, argv_a);

	char *arg;
	int new_args = 0;
	while ((arg = va_arg(ap, char *))) {
		str_array_insert(&argv_a->arr, 0, arg);
		new_args++;
	}

	va_end(ap);

	return new_args;
}

char **argv_array_detach(struct argv_array *argv_a, size_t *len)
{
	return str_array_detach(&argv_a->arr, len);
}

char *argv_array_collapse(struct argv_array *argv_a)
{
	return argv_array_collapse_delim(argv_a, " ");
}

char *argv_array_collapse_delim(struct argv_array *argv_a, const char *delim)
{
	struct str_array str_a = argv_a->arr;
	size_t len = 1;

	if (!str_a.strings || !str_a.len)
		return NULL;

	if (str_a.len > 1)
		len += (str_a.len - 1) * strlen(delim);

	for (size_t i = 0; i < str_a.len; i++)
		len += strlen(str_a.strings[i]);

	char *str = (char *)calloc(len, sizeof(char));
	if (str == NULL)
		FATAL(MEM_ALLOC_FAILED);

	strcat(str, str_a.strings[0]);
	for (size_t i = 1; i < str_a.len; i++) {
		strcat(str, delim);
		strcat(str, str_a.strings[i]);
	}

	return str;
}
