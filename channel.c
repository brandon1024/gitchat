//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>

#include "channel.h"
#include "usage.h"

static const struct usage_description channel_cmd_usage[] = {
        USAGE("git chat channel [-l | --list] [-r | --remotes] [-a | --all]"),
        USAGE("git chat channel [(-c | --create)] <name>"),
        USAGE("git chat channel (-s | --switch) <name>"),
        USAGE("git chat channel (-d | --delete) <name>"),
        USAGE_END()
};

static const struct option_description channel_cmd_options[] = {
        OPT_BOOL("l", "list", "list channel names"),
        OPT_BOOL("r", "remotes", "list remote-tracking channels"),
        OPT_BOOL("a", "all", "list all local and remote-tracking channels"),
        OPT_STRING("c", "create", "name", "create a new channel with the specified name"),
        OPT_STRING("s", "switch", "name", "switch to an existing channel"),
        OPT_STRING("d", "delete", "name", "delete a channel locally"),
        OPT_END()
};

int cmd_channel(int argc, char *argv[])
{
    show_channel_usage(NULL);
    return 0;
}

void show_channel_usage(const char *msg)
{
    show_usage_with_options(channel_cmd_usage, channel_cmd_options, msg);
}