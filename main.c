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

void show_usage() {
    printf("example");
}

int main(int argc, char *argv[])
{
    if(argc < 2) {
        show_usage();
    }

    if(!strcmp(argv[1], "channel")) {
        printf("channel");
    } else if(!strcmp(argv[1], "message")) {
        printf("message");
    } else if(!strcmp(argv[1], "publish")) {
        printf("publish");
    } else if(!strcmp(argv[1], "get")) {
        printf("get");
    } else {
        printf("unknown");
    }

    return 0;
}

