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
    show_publish_usage(NULL);
    return 0;
}

void show_publish_usage(const char *optional_message_format, ...)
{
    va_list varargs;
    va_start(varargs, optional_message_format);

    show_usage(publish_cmd_usage, optional_message_format);

    va_end(varargs);
}