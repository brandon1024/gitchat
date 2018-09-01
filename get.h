//
// Created by Brandon Richardson on 2018-08-19.
//

#ifndef GITCHAT_GET_H
#define GITCHAT_GET_H

/**
 * Entry for the get command.
 * */
int cmd_get(int argc, char *argv[]);

/**
 * Print usage and options of the get command to stdout, along with an optional message from a format string.
 *
 * See usage.h for more details.
 * */
void show_get_usage(const char *msg);

#endif //GITCHAT_GET_H
