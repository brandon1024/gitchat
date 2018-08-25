//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "channel.h"
#include "message.h"
#include "publish.h"
#include "get.h"
#include "read.h"
#include "usage.h"

static const struct usage_description main_cmd_usage[] = {
        USAGE("git chat <command> [<options>]"),
        USAGE_END()
};

static const struct option_description main_cmd_options[] = {
        OPT_CMD("channel", "TODO DESCRIPTION"),
        OPT_CMD("message", "TODO DESCRIPTION"),
        OPT_CMD("publish", "TODO DESCRIPTION"),
        OPT_CMD("get", "TODO DESCRIPTION"),
        OPT_CMD("read", "TODO DESCRIPTION"),
        OPT_END()
};

void show_main_usage(const char *msg)
{
    show_usage_with_options(main_cmd_usage, main_cmd_options, msg);
}

void show_version() {

}

int main(int argc, char *argv[])
{
    if(argc < 2) {
        show_main_usage(NULL);
        return 1;
    }

    if(!strcmp(argv[1], "channel")) {
        cmd_channel(argc - 2, argv + 2);
    } else if(!strcmp(argv[1], "message")) {
        cmd_message(argc - 2, argv + 2);
    } else if(!strcmp(argv[1], "publish")) {
        cmd_publish(argc - 2, argv + 2);
    } else if(!strcmp(argv[1], "get")) {
        cmd_get(argc - 2, argv + 2);
    } else if(!strcmp(argv[1], "read")) {
        cmd_read(argc - 2, argv + 2);
    } else {
        char *msg = (char *)malloc((strlen(argv[1]) + 26) * sizeof(char));
        sprintf(msg, "error: unknown command '%s'", argv[1]);
        show_main_usage(msg);
        free(msg);
    }

    return 0;
}
