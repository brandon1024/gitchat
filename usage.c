//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>

#include "options.h"
#include "usage.h"

void show_usage(const char * const cmd_usage[], const char *err_msg)
{
    if(err_msg != NULL) {
        printf("%s\n", err_msg);
    }

    int index = 0;

    while(cmd_usage[index]) {
        printf("%s\n", cmd_usage[index]);
        index++;
    }

    printf("\n");
}

void usage_with_options(const char * const cmd_usage[], const struct option *opts, const char *err_msg)
{
    show_usage(cmd_usage, err_msg);
    show_options(opts);
}