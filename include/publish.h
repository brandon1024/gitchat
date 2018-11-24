#ifndef GITCHAT_PUBLISH_H
#define GITCHAT_PUBLISH_H

/**
 * Entry for the publish command.
 * */
int cmd_publish(int argc, char *argv[]);

/**
 * Print usage and options of the publish command, along with an optional message from a format string.
 *
 * See usage.h for more details.
 * */
void show_publish_usage(int err, const char *optional_message_format, ...);

#endif //GITCHAT_PUBLISH_H
