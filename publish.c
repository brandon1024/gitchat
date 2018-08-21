//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>

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

void show_publish_usage(const char *msg)
{
    show_usage(publish_cmd_usage, msg);
}