#include <string.h>

#include "str-array.h"
#include "utils.h"

#define BUFF_SLOP 8

void str_array_init(struct str_array *str_a)
{
	*str_a = (struct str_array){ NULL, 0, 0 };
}

void str_array_grow(struct str_array *str_a, size_t size)
{
	if ((size - 1) < str_a->len)
		return;

	str_a->alloc = size;
	str_a->strings = (char **)realloc(str_a->strings,
			str_a->alloc * sizeof(char *));
	if (!str_a->strings)
		FATAL(MEM_ALLOC_FAILED);
}

void str_array_release(struct str_array *str_a)
{
	for (size_t i = 0; i < str_a->len; i++)
		free(str_a->strings[i]);

	free(str_a->strings);
	str_array_init(str_a);
}

char *str_array_get(struct str_array *str_a, size_t pos)
{
	if (pos >= str_a->len)
		return NULL;

	return str_a->strings[pos];
}

int str_array_set(struct str_array *str_a, char *str, size_t pos)
{
	if (pos >= str_a->len)
		return 1;

	char *duplicated_str = strdup(str);
	if (!duplicated_str)
		FATAL(MEM_ALLOC_FAILED);

	return str_array_set_nodup(str_a, duplicated_str, pos);
}

int str_array_set_nodup(struct str_array *str_a, char *str, size_t pos)
{
	if (pos >= str_a->len)
		return 1;

	free(str_a->strings[pos]);
	str_a->strings[pos] = str;
	return 0;
}

int str_array_push(struct str_array *str_a, ...)
{
	va_list ap;
	va_start(ap, str_a);
	int new_args = str_array_vpush(str_a, ap);
	va_end(ap);

	return new_args;
}

int str_array_vpush(struct str_array *str_a, va_list args)
{
	int new_strings = 0;

	char *arg;
	while ((arg = va_arg(args, char *))) {
		if ((str_a->len + 2) >= str_a->alloc)
			str_array_grow(str_a, str_a->alloc + BUFF_SLOP);

		char *str = strdup(arg);
		if (!str)
			FATAL(MEM_ALLOC_FAILED);

		str_a->strings[str_a->len++] = str;
		str_a->strings[str_a->len] = NULL;
		new_strings++;
	}

	return new_strings;
}

size_t str_array_insert(struct str_array *str_a, size_t pos, char *str)
{
	char *duplicated_str = strdup(str);
	if (!duplicated_str)
		FATAL(MEM_ALLOC_FAILED);

	return str_array_insert_nodup(str_a, pos, duplicated_str);
}

size_t str_array_insert_nodup(struct str_array *str_a, size_t pos, char *str)
{
	if ((str_a->len + 2) >= str_a->alloc)
		str_array_grow(str_a, str_a->alloc + BUFF_SLOP);

	str_a->strings[str_a->len + 1] = NULL;

	if (pos < str_a->len) {
		for (size_t i = str_a->len; i > pos; i--)
			str_a->strings[i] = str_a->strings[i - 1];
	} else {
		pos = str_a->len;
	}

	str_a->strings[pos] = str;
	str_a->len++;

	return pos;
}

static int str_comparator(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

void str_array_sort(struct str_array *str_a)
{
	qsort(str_a->strings, str_a->len, sizeof(const char *), str_comparator);
}

char *str_array_remove(struct str_array *str_a, size_t pos)
{
	if (pos >= str_a->len)
		return NULL;

	char *removed_str = str_a->strings[pos];
	for (size_t i = pos; i < str_a->len - 1; i++)
		str_a->strings[i] = str_a->strings[i+1];

	str_a->len--;
	str_a->strings[str_a->len] = NULL;
	return removed_str;
}

char **str_array_detach(struct str_array *str_a, size_t *len)
{
	char **arr = str_a->strings;
	*len = str_a->len;

	str_array_init(str_a);

	return arr;
}
