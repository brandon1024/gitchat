//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "usage.h"

#define USAGE_OPTIONS_WIDTH     24
#define USAGE_OPTIONS_GAP        2

void show_usage(const struct usage_description *cmd_usage, const char *optional_message_format, ...)
{
    va_list varargs;
    va_start(varargs, optional_message_format);
    variadic_show_usage(cmd_usage, optional_message_format, varargs);
    va_end(varargs);
}

void variadic_show_usage(const struct usage_description *cmd_usage, const char *optional_message_format,
        va_list varargs)
{
    if(optional_message_format != NULL) {
        vprintf(optional_message_format, varargs);
        printf("\n");
    }

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
void show_options(const struct option_description *opts)
{
    int index = 0;
    while(opts[index].type != OPTION_END) {
        struct option_description opt = opts[index++];
        int printed_chars = 0;

        printed_chars += printf("    ");
        if(opt.type == OPTION_BOOL_T) {
            if(opt.s_flag != 0)
                printed_chars += printf("-%c", opt.s_flag);

            if(opt.s_flag != 0 && opt.l_flag != NULL)
                printed_chars += printf(", --%s", opt.l_flag);
            else if(opt.l_flag != NULL)
                printed_chars += printf("--%s", opt.l_flag);
        } else if(opt.type == OPTION_INT_T) {
            if(opt.s_flag != 0)
                printed_chars += printf("-%c=<n>", opt.s_flag);

            if(opt.s_flag != 0 && opt.l_flag != NULL)
                printed_chars += printf(", --%s=<n>", opt.l_flag);
            else if(opt.l_flag != NULL)
                printed_chars += printf("--%s=<n>", opt.l_flag);
        } else if(opt.type == OPTION_STRING_T) {
            if(opt.s_flag != 0)
                printed_chars += printf("-%c", opt.s_flag);

            if(opt.s_flag != 0 && opt.l_flag != NULL)
                printed_chars += printf(", --%s", opt.l_flag);
            else if(opt.l_flag != NULL)
                printed_chars += printf("--%s", opt.l_flag);

            printed_chars += printf(" <%s>", opt.str_name);
        } else if(opt.type == OPTION_COMMAND_T)
            printed_chars += printf("%s", opt.str_name);

        if(printed_chars >= (USAGE_OPTIONS_WIDTH - USAGE_OPTIONS_GAP))
            printf("\n%*s%s\n", USAGE_OPTIONS_WIDTH, "", opt.desc);
        else
            printf("%*s%s\n", USAGE_OPTIONS_WIDTH - printed_chars, "", opt.desc);
    }

    printf("\n");
}

void show_usage_with_options(const struct usage_description *cmd_usage, const struct option_description *opts,
        const char *optional_message_format, ...)
{
    va_list varargs;
    va_start(varargs, optional_message_format);

    variadic_show_usage_with_options(cmd_usage, opts, optional_message_format, varargs);

    va_end(varargs);
}

void variadic_show_usage_with_options(const struct usage_description *cmd_usage, const struct option_description *opts,
                             const char *optional_message_format, va_list varargs)
{
    variadic_show_usage(cmd_usage, optional_message_format, varargs);
    show_options(opts);
}

int argument_matches_option(const char *arg, struct option_description description)
{
    //If option is of type command
    if(description.type == OPTION_COMMAND_T) {
        return !strcmp(arg, description.str_name);
    }

    /*
     * If argument is less than two characters in length, or is not prefixed by a dash, return false
     * as it is not a valid command line flag
     */
    if(strlen(arg) <= 1 || arg[0] != '-') {
        return false;
    }

    //If argument is in long format
    if(arg[0] == '-' && arg[1] == '-') {
        return !strcmp(arg + 2, description.l_flag);
    }

    //If argument is in short combined boolean format
    if(strlen(arg) > 2) {
        return strchr(arg + 1, description.s_flag) != NULL && description.type == OPTION_BOOL_T;
    }

    //If argument is in short format
    return arg[1] == description.s_flag;
}
