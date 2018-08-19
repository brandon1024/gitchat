//
// Created by Brandon Richardson on 2018-08-19.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "channel.h"
#include "message.h"
#include "publish.h"
#include "get.h"
#include "read.h"

void show_main_usage(const char *msg)
{
    if(msg != NULL) {
        printf("%s\n", msg);
    }

    printf("usage: git chat <command> [<options>]\n\n");
    printf("These are common chat commands used in various situations:\n\n");
    printf("\t%s\t%s\n", "channel", "");
    printf("\t%s\t%s\n", "message", "");
    printf("\t%s\t%s\n", "get", "");
    printf("\t%s\t%s\n", "publish", "");
    printf("\t%s\t%s\n", "read", "");
    printf("\n");
}

int main(int argc, char *argv[])
{
    if(argc < 2) {
        show_main_usage(NULL);
        return 1;
    }

    if(!strcmp(argv[1], "channel")) {
        cmd_channel(argc - 2, argv + 2);
    } else if(!strcmp(argv[1], "message")) {
        cmd_message(argc - 2, argv + 2);
    } else if(!strcmp(argv[1], "publish")) {
        cmd_publish(argc - 2, argv + 2);
    } else if(!strcmp(argv[1], "get")) {
        cmd_get(argc - 2, argv + 2);
    } else if(!strcmp(argv[1], "read")) {
        cmd_read(argc - 2, argv + 2);
    } else {
        char *msg = (char *)malloc((strlen(argv[1]) + 26) * sizeof(char));
        sprintf(msg, "error: unknown command '%s'", argv[1]);
        show_main_usage(msg);
        free(msg);
    }

    return 0;
}

