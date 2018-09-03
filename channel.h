//
// Created by Brandon Richardson on 2018-08-19.
//

#ifndef GITCHAT_CHANNEL_H
#define GITCHAT_CHANNEL_H

/**
 * Entry for the channel command.
 * */
int cmd_channel(int argc, char *argv[]);

/**
 * Print usage and options of the channel command, along with an optional message from a format string.
 *
 * See usage.h for more details.
 * */
void show_channel_usage(int err, const char *optional_message_format, ...);

#endif //GITCHAT_CHANNEL_H
