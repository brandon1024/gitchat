//
// Created by Brandon Richardson on 2018-08-19.
//

#ifndef GITCHAT_READ_H
#define GITCHAT_READ_H

/**
 * Entry for the read command.
 * */
int cmd_read(int argc, char *argv[]);

/**
 * Print usage and options of the read command to stdout, along with an optional message from a format string.
 *
 * See usage.h for more details.
 * */
void show_read_usage(int err, const char *optional_message_format, ...);

#endif //GITCHAT_READ_H
