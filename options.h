//
// Created by Brandon Richardson on 2018-08-19.
//

#ifndef GITCHAT_PARSE_OPTIONS_H
#define GITCHAT_PARSE_OPTIONS_H

enum opt_type {
    OPTION_BOOL_T,
    OPTION_INT_T,
    OPTION_STRING_T,
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

#define OPT_BOOL(S,L,D)         { (S), (L), NULL, (D), OPTION_BOOL_T }
#define OPT_INT(S,L,D)          { (S), (L), NULL, (D), OPTION_INT_T }
#define OPT_STRING(S,L,N,D)     { (S), (L), (N), (D), OPTION_STRING_T }
#define OPT_END()               { NULL, NULL, NULL, NULL, OPTION_END }

void show_options(const struct option *opts);

#endif //GITCHAT_PARSE_OPTIONS_H
