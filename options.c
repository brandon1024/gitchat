//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"

void show_options(const struct option *opts) {
    int index = 0;

    while(opts[index].type != OPTION_END) {
        struct option opt = opts[index++];

        if(opt.type == OPTION_BOOL_T) {
            if(opt.l_flag != NULL) {
                printf("\t-%s, --%s\t\t%s\n", opt.s_flag, opt.l_flag, opt.desc);
            } else {
                printf("\t-%s\t\t%s\n", opt.s_flag, opt.desc);
            }
        } else if(opt.type == OPTION_INT_T) {
            if(opt.l_flag != NULL) {
                printf("\t-%s=<n>, --%s=<n>\t\t%s\n", opt.s_flag, opt.l_flag, opt.desc);
            } else {
                printf("\t-%s=<n>\t\t%s\n", opt.s_flag, opt.desc);
            }
        } else if(opt.type == OPTION_STRING_T) {
            if(opt.l_flag != NULL) {
                printf("\t-%s, --%s <%s>\t\t%s\n", opt.s_flag, opt.l_flag, opt.str_name, opt.desc);
            } else {
                printf("\t-%s <%s>\t\t%s\n", opt.s_flag, opt.str_name, opt.desc);
            }
        }
    }
}