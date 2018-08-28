//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>
#include <stdarg.h>

#include "read.h"
#include "usage.h"

static const struct usage_description read_cmd_usage[] = {
        USAGE("git chat read [<options>] (<commit-hash> | <filename>)"),
        USAGE("git chat read [(-n | --max-count) <number>] [--dir (ASC | DSC)] [--oneline] [--short]"),
        USAGE("git chat read [--from <email>] [--to <email>]"),
        USAGE("git chat read [--pull-new]"),
        USAGE_END()
};

static const struct option_description read_cmd_options[] = {
        OPT_INT('n', "max-count", "TODO DESCRIPTION"),
        OPT_LONG_STRING("dir", "direction", "TODO DESCRIPTION"),
        OPT_LONG_BOOL("oneline", "TODO DESCRIPTION"),
        OPT_LONG_BOOL("short", "TODO DESCRIPTION"),
        OPT_LONG_STRING("from", "email", "TODO DESCRIPTION"),
        OPT_LONG_STRING("to", "email", "TODO DESCRIPTION"),
        OPT_LONG_BOOL("pull-new", "TODO DESCRIPTION"),
        OPT_END()
};

int cmd_read(int argc, char *argv[]) {
    show_read_usage(NULL);
    return 0;
}

void show_read_usage(const char *optional_message_format, ...)
{
    va_list varargs;
    va_start(varargs, optional_message_format);

    variadic_show_usage_with_options(read_cmd_usage, read_cmd_options, optional_message_format, varargs);

    va_end(varargs);
}