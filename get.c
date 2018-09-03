//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>

#include "get.h"
#include "usage.h"

static const struct usage_description get_cmd_usage[] = {
        USAGE("git chat get"),
        USAGE_END()
};

int cmd_get(int argc, char *argv[])
{
    show_get_usage(0, NULL);
    return 0;
}

void show_get_usage(int err, const char *optional_message_format, ...)
{
    va_list varargs;
    va_start(varargs, optional_message_format);

    variadic_show_usage(get_cmd_usage, optional_message_format, varargs, err);

    va_end(varargs);
}