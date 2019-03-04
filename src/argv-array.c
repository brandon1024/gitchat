#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "argv-array.h"
#include "utils.h"

#define BUFF_SLOP 8

void argv_array_init(struct argv_array *argv_a)
{
	argv_a->argv = NULL;
	argv_a->argc = 0;
	argv_a->alloc = 0;
}

void argv_array_release(struct argv_array *argv_a)
{
	for(size_t i = 0; i < argv_a->argc; i++)
		free(argv_a->argv[i]);

	free(argv_a->argv);
	argv_array_init(argv_a);
}

int argv_array_push(struct argv_array *argv_a, ...)
{
	va_list ap;
	va_start(ap, argv_a);

	char *arg;
	int new_args = 0;
	while((arg = va_arg(ap, char *))) {
		if(argv_a->argc + 2 >= argv_a->alloc) {
			argv_a->alloc += sizeof(char *) * BUFF_SLOP;
			argv_a->argv = (char **)realloc(argv_a->argv, argv_a->alloc);
			if(argv_a->argv == NULL)
				FATAL("Unable to allocate memory.");
		}

		argv_a->argv[argv_a->argc++] = strdup(arg);
		argv_a->argv[argv_a->argc] = NULL;
		new_args++;
	 }

	va_end(ap);

	return new_args;
}

char *argv_array_pop(struct argv_array *argv_a)
{
	if(argv_a->argc == 0)
		return NULL;

	char *top = argv_a->argv[argv_a->argc - 1];
	argv_a->argv[argv_a->argc - 1] = NULL;
	argv_a->argc--;

	return top;
}

int argv_array_prepend(struct argv_array *argv_a, ...)
{
	struct argv_array tmp;
	argv_array_init(&tmp);

	va_list ap;
	va_start(ap, argv_a);

	char *arg;
	int new_args = 0;
	while((arg = va_arg(ap, char *))) {
		argv_array_push(&tmp, arg, NULL);
		new_args++;
	}

	va_end(ap);

	size_t len = argv_a->argc + tmp.argc;
	char **tmp_arr = (char **)malloc(sizeof(char *) * (len + BUFF_SLOP));
	if(argv_a->argv == NULL)
		FATAL("Unable to allocate memory.");

	memcpy(tmp_arr, tmp.argv, sizeof(char *) * tmp.argc);
	memcpy(tmp_arr + tmp.argc, argv_a->argv, sizeof(char *) * argv_a->argc);
	tmp_arr[len] = NULL;

	free(argv_a->argv);
	free(tmp.argv);

	argv_a->argc = len;
	argv_a->alloc = len + BUFF_SLOP;
	argv_a->argv = tmp_arr;

	return new_args;
}

char **argv_array_detach(struct argv_array *argv_a, size_t *len)
{
	char **arr = argv_a->argv;
	*len = argv_a->argc;

	argv_array_init(argv_a);

	return arr;
}

char *argv_array_collapse(struct argv_array *argv_a)
{
	return argv_array_collapse_delim(argv_a, " ");
}

char *argv_array_collapse_delim(struct argv_array *argv_a, const char *delim)
{
	size_t len = 1;

	if(!argv_a->argc)
		return NULL;

	if(argv_a->argc > 1)
		len += (argv_a->argc - 1) * strlen(delim);

	for(size_t i = 0; i < argv_a->argc; i++)
		len += strlen(argv_a->argv[i]);

	char *str = (char *)calloc(len, sizeof(char));
	if(str == NULL)
		FATAL("Unable to allocate memory.");

	strcat(str, argv_a->argv[0]);
	for(size_t i = 1; i < argv_a->argc; i++) {
		strcat(str, delim);
		strcat(str, argv_a->argv[i]);
	}

	return str;
}
