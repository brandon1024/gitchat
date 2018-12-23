#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "version.h"
#include "channel.h"
#include "message.h"
#include "publish.h"
#include "get.h"
#include "read.h"
#include "usage.h"

static const struct usage_description main_cmd_usage[] = {
        USAGE("git chat <command> [<options>]"),
        USAGE("git chat [-h | --help]"),
        USAGE("git chat (-v | --version)"),
        USAGE_END()
};

static const struct option_description main_cmd_options[] = {
        OPT_CMD("channel", "Create and manage communication channels"),
        OPT_CMD("message", "Create messages"),
        OPT_CMD("publish", "Publish messages to the remote server"),
        OPT_CMD("get", "Download messages"),
        OPT_CMD("read", "Display, format and read messages"),
        OPT_BOOL('h', "help", "Show usage and exit"),
        OPT_BOOL('v', "version", "Output version information and exit"),
        OPT_END()
};

/* Function Prototypes */
void show_main_usage(int err, const char *optional_message_format, ...);
void show_version(void);

/* Public Functions */
int main(int argc, char *argv[])
{
    //Show usage and return if no arguments provided
    if(argc < 2) {
        show_main_usage(0, NULL);
        return 0;
    }

    //Show usage and return if -h argument
    if(argument_matches_option(argv[1], main_cmd_options[5])) {
        show_main_usage(0, NULL);
        return 0;
    }

    //Show version and return if -v argument
    if(argument_matches_option(argv[1], main_cmd_options[6])) {
        show_version();
        return 0;
    }

    //delegate commands
    if(argument_matches_option(argv[1], main_cmd_options[0]))
        return cmd_channel(argc - 2, argv + 2);

    if(argument_matches_option(argv[1], main_cmd_options[1]))
        return cmd_message(argc - 2, argv + 2);

    if(argument_matches_option(argv[1], main_cmd_options[2]))
        return cmd_publish(argc - 2, argv + 2);

    if(argument_matches_option(argv[1], main_cmd_options[3]))
        return cmd_get(argc - 2, argv + 2);

    if(argument_matches_option(argv[1], main_cmd_options[4]))
        return cmd_read(argc - 2, argv + 2);

    show_main_usage(1, "error: unknown command '%s'", argv[1]);

    return 1;
}

void show_main_usage(int err, const char *optional_message_format, ...)
{
    va_list varargs;
    va_start(varargs, optional_message_format);

    variadic_show_usage_with_options(main_cmd_usage, main_cmd_options, optional_message_format, varargs, err);

    va_end(varargs);
}

void show_version(void)
{
    fprintf(stdout, "git-chat version %u.%u\n", GITCHAT_VERSION_MAJOR, GITCHAT_VERSION_MINOR);
}
