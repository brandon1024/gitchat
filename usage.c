//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>
#include <string.h>

#include "usage.h"

void show_usage(const struct usage_description *cmd_usage, const char *optional_message)
{
    if(optional_message != NULL)
        printf("%s\n", optional_message);

    int index = 0;
    while(cmd_usage[index].usage_desc != NULL) {
        if(index == 0)
            printf("%s %s\n", "usage:", cmd_usage[index].usage_desc);
        else
            printf("%s %s\n", "   or:", cmd_usage[index].usage_desc);

        index++;
    }

    printf("\n");
}


/*
 * Print options to stdout.
 * */
void show_options(const struct option *opts) {
    int index = 0;
    while(opts[index].type != OPTION_END) {
        struct option opt = opts[index++];

        printf("    ");
        if(opt.type == OPTION_BOOL_T) {
            if(opt.s_flag != NULL)
                printf("-%s", opt.s_flag);

            if(opt.s_flag != NULL && opt.l_flag != NULL)
                printf(", --%s", opt.l_flag);
            else if(opt.l_flag != NULL)
                printf("--%s", opt.l_flag);
        } else if(opt.type == OPTION_INT_T) {
            if(opt.s_flag != NULL)
                printf("-%s=<n>", opt.s_flag);

            if(opt.s_flag != NULL && opt.l_flag != NULL)
                printf(", --%s=<n>", opt.l_flag);
            else if(opt.l_flag != NULL)
                printf("--%s=<n>", opt.l_flag);
        } else if(opt.type == OPTION_STRING_T) {
            if(opt.s_flag != NULL)
                printf("-%s", opt.s_flag);

            if(opt.s_flag != NULL && opt.l_flag != NULL)
                printf(", --%s", opt.l_flag);
            else if(opt.l_flag != NULL)
                printf("--%s", opt.l_flag);

            printf(" <%s>", opt.str_name);
        }

        printf("    %s\n", opt.desc);
    }
}

void usage_with_options(const struct usage_description *cmd_usage, const struct option *opts, const char *optional_message)
{
    show_usage(cmd_usage, optional_message);
    show_options(opts);
}
