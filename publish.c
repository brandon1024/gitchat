//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>
#include <stdarg.h>

#include "publish.h"
#include "usage.h"

static const struct usage_description publish_cmd_usage[] = {
        USAGE("git chat publish"),
        USAGE_END()
};

int cmd_publish(int argc, char *argv[]) {
    show_publish_usage(0, NULL);
    return 0;
}

void show_publish_usage(int err, const char *optional_message_format, ...)
{
    va_list varargs;
    va_start(varargs, optional_message_format);

    variadic_show_usage(publish_cmd_usage, optional_message_format, varargs, err);

    va_end(varargs);
}