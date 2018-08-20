//
// Created by Brandon Richardson on 2018-08-19.
//

#ifndef GITCHAT_USAGE_H
#define GITCHAT_USAGE_H

enum opt_type {
    OPTION_BOOL_T,
    OPTION_INT_T,
    OPTION_STRING_T,
    OPTION_COMMAND_T,
    OPTION_END
};

struct option;
struct option {
    const char *s_flag;
    const char *l_flag;
    const char *str_name;
    const char *desc;
    enum opt_type type;
};

struct usage_description;
struct usage_description {
    const char *usage_desc;
};

#define OPT_SHORT_BOOL(S,D)     { (S), NULL,  NULL, (D), OPTION_BOOL_T }
#define OPT_SHORT_INT(S,D)      { (S), NULL,  NULL, (D), OPTION_INT_T }
#define OPT_SHORT_STRING(S,N,D) { (S), NULL,  (N), (D), OPTION_STRING_T }
#define OPT_LONG_BOOL(L,D)      { NULL, (L),  NULL, (D), OPTION_BOOL_T }
#define OPT_LONG_INT(L,D)       { NULL, (L),  NULL, (D), OPTION_INT_T }
#define OPT_LONG_STRING(L,N,D)  { NULL, (L),  (N), (D), OPTION_STRING_T }
#define OPT_BOOL(S,L,D)         { (S), (L), NULL, (D), OPTION_BOOL_T }
#define OPT_INT(S,L,D)          { (S), (L), NULL, (D), OPTION_INT_T }
#define OPT_STRING(S,L,N,D)     { (S), (L), (N), (D), OPTION_STRING_T }
#define OPT_CMD(N,D)            { NULL, NULL, (N), (D), OPTION_COMMAND_T }
#define OPT_END()               { NULL, NULL, NULL, NULL, OPTION_END }

#define USAGE(DESC)             { (DESC) }
#define USAGE_END()             { NULL }

void show_options(const struct option *opts);
void show_usage(const struct usage_description *cmd_usage, const char *optional_message);
void usage_with_options(const struct usage_description *cmd_usage, const struct option *opts, const char *optional_message);

#endif //GITCHAT_USAGE_H
