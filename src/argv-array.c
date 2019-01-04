#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "argv-array.h"

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

void argv_array_push(struct argv_array *argv_a, ...)
{
    va_list ap;
    va_start(ap, argv_a);

    const char *arg;
    while((arg = va_arg(ap, char *))) {
        if(argv_a->argc + 2 >= argv_a->alloc) {
            argv_a->alloc += 8;
            argv_a->argv = (char **)realloc(argv_a->argv, argv_a->alloc);
            if(argv_a->argv == NULL) {
                perror("Fatal Error: Unable to allocate memory.\n");
                exit(EXIT_FAILURE);
            }
        }

        argv_a->argv[argv_a->argc++] = strdup(arg);
        argv_a->argv[argv_a->argc] = NULL;
     }

    va_end(ap);
}

void argv_array_pop(struct argv_array *argv_a)
{
    if(argv_a->argc == 0)
        return;

    free(argv_a->argv[argv_a->argc - 1]);
    argv_a->argv[argv_a->argc - 1] = NULL;
    argv_a->argc--;
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
    size_t len = (argv_a->argc > 1) ? argv_a->argc - 1 : 0;

    for(size_t index = 0; index < argv_a->argc; index++)
        len += strlen(argv_a->argv[index]);

    char *str = (char *)calloc(len + 1, sizeof(char));
    if(str == NULL) {
        perror("Fatal Error: Unable to allocate memory.\n");
        exit(EXIT_FAILURE);
    }

    if(argv_a->argc >= 1)
        strcat(str, argv_a->argv[0]);

    for(size_t index = 1; index < argv_a->argc; index++) {
        strcat(str, " ");
        strcat(str, argv_a->argv[index]);
    }

    return str;
}