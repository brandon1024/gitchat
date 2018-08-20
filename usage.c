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
    int flag_padding_len = 0;
    while(opts[index].type != OPTION_END) {
        struct option opt = opts[index++];
        int opt_padding_len = 0;

        if(opt.s_flag != NULL)
            opt_padding_len += strlen(opt.s_flag);
        if(opt.l_flag != NULL)
            opt_padding_len += strlen(opt.l_flag);
        if(opt.str_name != NULL)
            opt_padding_len += strlen(opt.str_name);

        if(opt.type == OPTION_BOOL_T) {
            if(opt.s_flag != NULL)
                opt_padding_len += 1;

            if(opt.s_flag != NULL && opt.l_flag != NULL)
                opt_padding_len += 4;
            else if(opt.l_flag != NULL)
                opt_padding_len += 2;
        } else if(opt.type == OPTION_INT_T) {
            if(opt.s_flag != NULL)
                opt_padding_len += 5;

            if(opt.s_flag != NULL && opt.l_flag != NULL)
                opt_padding_len += 8;
            else if(opt.l_flag != NULL)
                opt_padding_len += 6;
        } else if(opt.type == OPTION_STRING_T) {
            if(opt.s_flag != NULL)
                opt_padding_len += 1;

            if(opt.s_flag != NULL && opt.l_flag != NULL)
                opt_padding_len += 4;
            else if(opt.l_flag != NULL)
                opt_padding_len += 2;

            opt_padding_len += 3;
        } else if(opt.type == OPTION_COMMAND_T) {
            opt_padding_len += 0;
        }

        flag_padding_len = opt_padding_len > flag_padding_len ? opt_padding_len : flag_padding_len;
    }

    index = 0;
    while(opts[index].type != OPTION_END) {
        struct option opt = opts[index++];
        int printed_chars = 0;

        printf("    ");
        if(opt.type == OPTION_BOOL_T) {
            if(opt.s_flag != NULL)
                printed_chars += printf("-%s", opt.s_flag);

            if(opt.s_flag != NULL && opt.l_flag != NULL)
                printed_chars += printf(", --%s", opt.l_flag);
            else if(opt.l_flag != NULL)
                printed_chars += printf("--%s", opt.l_flag);
        } else if(opt.type == OPTION_INT_T) {
            if(opt.s_flag != NULL)
                printed_chars += printf("-%s=<n>", opt.s_flag);

            if(opt.s_flag != NULL && opt.l_flag != NULL)
                printed_chars += printf(", --%s=<n>", opt.l_flag);
            else if(opt.l_flag != NULL)
                printed_chars += printf("--%s=<n>", opt.l_flag);
        } else if(opt.type == OPTION_STRING_T) {
            if(opt.s_flag != NULL)
                printed_chars += printf("-%s", opt.s_flag);

            if(opt.s_flag != NULL && opt.l_flag != NULL)
                printed_chars += printf(", --%s", opt.l_flag);
            else if(opt.l_flag != NULL)
                printed_chars += printf("--%s", opt.l_flag);

            printed_chars += printf(" <%s>", opt.str_name);
        } else if(opt.type == OPTION_COMMAND_T) {
            printed_chars += printf("%s", opt.str_name);
        }

        printf("    ");
        printf("%*s%s\n", flag_padding_len - printed_chars, "", opt.desc);
    }

    printf("\n");
}

void usage_with_options(const struct usage_description *cmd_usage, const struct option *opts, const char *optional_message)
{
    show_usage(cmd_usage, optional_message);
    show_options(opts);
}
