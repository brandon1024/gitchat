//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>

#include "message.h"
#include "usage.h"

static const struct usage_description message_cmd_usage[] = {
        USAGE("git chat message [-p | --public] [--recipient <email>...] [(-c | --comment) <comment>] [-m | --message] <message>"),
        USAGE("git chat message (-s | --symmetric) [--password <password>] <message>"),
        USAGE("git chat message (-f | --file) <filename>"),
        USAGE_END()
};

static const struct option_description message_cmd_options[] = {
        OPT_BOOL("p", "public", "TODO DESCRIPTION"),
        OPT_LONG_STRING("recipient", "email", "TODO DESCRIPTION"),
        OPT_STRING("c", "comment", "comment", "TODO DESCRIPTION"),
        OPT_STRING("m", "message", "message", "TODO DESCRIPTION"),
        OPT_BOOL("s", "symmetric", "TODO DESCRIPTION"),
        OPT_LONG_STRING("password", "password", "TODO DESCRIPTION"),
        OPT_STRING("f", "file", "filename", "TODO DESCRIPTION"),
        OPT_END()
};

int cmd_message(int argc, char *argv[]) {
    show_message_usage(NULL);
    return 0;
}

void show_message_usage(const char *msg)
{
    show_usage_with_options(message_cmd_usage, message_cmd_options, msg);
}