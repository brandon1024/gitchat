//
// Created by Brandon Richardson on 2018-08-19.
//

#ifndef GITCHAT_MESSAGE_H
#define GITCHAT_MESSAGE_H

/**
 * Entry for the message command.
 * */
int cmd_message(int argc, char *argv[]);

/**
 * Print usage and options of the message command to stdout, along with an optional message from a format string.
 *
 * See usage.h for more details.
 * */
void show_message_usage(const char *optional_message_format, ...);

#endif //GITCHAT_MESSAGE_H
