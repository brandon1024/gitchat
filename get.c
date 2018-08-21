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
    show_get_usage(NULL);
    return 0;
}

void show_get_usage(const char *msg)
{
    show_usage(get_cmd_usage, msg);
}