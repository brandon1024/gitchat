//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "channel.h"
#include "usage.h"

#define LIST_MODE_LOCAL 0x1
#define LIST_MODE_REMOTE 0x2
#define ACTION_MODE_CREATE 0x1
#define ACTION_MODE_SWITCH 0x2
#define ACTION_MODE_DELETE 0x4
#define ACTION_MODE_GPGIMPORT 0x8

static const struct usage_description channel_cmd_usage[] = {
        USAGE("git chat channel [-l | --list] [-r | --remotes] [-a | --all]"),
        USAGE("git chat channel [(-c | --create)] <name>"),
        USAGE("git chat channel (-s | --switch) <name>"),
        USAGE("git chat channel (-d | --delete) <name>"),
        USAGE("git chat channel --gpg-import-key"),
        USAGE_END()
};

static const struct option_description channel_cmd_options[] = {
        OPT_BOOL("l", "list", "list channel names"),
        OPT_BOOL("r", "remotes", "list remote-tracking channels"),
        OPT_BOOL("a", "all", "list all local and remote-tracking channels"),
        OPT_STRING("c", "create", "name", "create a new channel with the specified name"),
        OPT_STRING("s", "switch", "name", "switch to an existing channel"),
        OPT_STRING("d", "delete", "name", "delete a channel locally"),
        OPT_LONG_BOOL("gpg-import-key", "import public gpg key into channel"),
        OPT_END()
};

/* Function Prototypes */
static int list_channels(unsigned char scope);
static int import_gpg_key_to_channel();
static int create_channel(const char *channel_name);
static int switch_to_channel(const char *channel_name);
static int delete_channel(const char *channel_name);

/* Public Functions */
int cmd_channel(int argc, char *argv[])
{
    unsigned char list_mode = 0;
    unsigned char action_mode = 0;
    const char *target = NULL;

    for(int arg_index = 0; arg_index < argc; arg_index++) {
        if(argument_matches_option(argv[arg_index], channel_cmd_options[0])) {
            list_mode |= LIST_MODE_LOCAL;
        } else if(argument_matches_option(argv[arg_index], channel_cmd_options[1])) {
            list_mode |= LIST_MODE_REMOTE;
        } else if(argument_matches_option(argv[arg_index], channel_cmd_options[2])) {
            list_mode |= LIST_MODE_LOCAL | LIST_MODE_REMOTE;
        } else if(argument_matches_option(argv[arg_index], channel_cmd_options[3])) {
            if((argc-1) <= arg_index+1) {
                show_channel_usage("fatal: invalid usage of %s. no channel name specified.", "test");
                return 1;
            }

            arg_index++;
            if(1) {

            }

            action_mode |= ACTION_MODE_CREATE;
            target = argv[++arg_index];
        } else if(argument_matches_option(argv[arg_index], channel_cmd_options[4])) {
            action_mode |= ACTION_MODE_SWITCH;
            target = argv[++arg_index];
        } else if(argument_matches_option(argv[arg_index], channel_cmd_options[5])) {
            action_mode |= ACTION_MODE_DELETE;
            target = argv[++arg_index];
        } else if(argument_matches_option(argv[arg_index], channel_cmd_options[6])) {
            action_mode |= ACTION_MODE_GPGIMPORT;
        } else {
            //implicit create
            action_mode |= ACTION_MODE_CREATE;
            target = argv[++arg_index];
        }
    }

    if(!list_mode) {
        show_channel_usage(NULL);
        return 0;
    }

    return list_channels(list_mode);
}

void show_channel_usage(const char *optional_message_format, ...)
{
    va_list varargs;
    va_start(varargs, optional_message_format);

    show_usage_with_options(channel_cmd_usage, channel_cmd_options, optional_message_format, varargs);

    va_end(varargs);
}

/* Internal Functions */
static int list_channels(unsigned char scope) {
    if(scope == LIST_MODE_LOCAL) {
        return 0; //TODO
    }

    if(scope == LIST_MODE_REMOTE) {
        return 0; //TODO
    }

    if(scope == (LIST_MODE_LOCAL | LIST_MODE_REMOTE)) {
        return 0; //TODO
    }

    return 1;
}

static int import_gpg_key_to_channel() {
    return 0; //TODO
}

static int create_channel(const char *channel_name) {
    return 0; //TODO
}

static int switch_to_channel(const char *channel_name) {
    return 0; //TODO
}

static int delete_channel(const char *channel_name) {
    return 0; //TODO
}