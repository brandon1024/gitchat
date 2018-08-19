//
// Created by Brandon Richardson on 2018-08-19.
//

#ifndef GITCHAT_USAGE_H
#define GITCHAT_USAGE_H

void show_usage(const char * const cmd_usage[], const char *err_msg);
void usage_with_options(const char * const cmd_usage[], const struct option *opts, const char *err_msg);

#endif //GITCHAT_USAGE_H
