#include <stdio.h>
#include "parse-options.h"

static const struct usage_string publish_cmd_usage[] = {
		USAGE("git chat publish"),
		USAGE_END()
};

int cmd_publish(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	show_usage(publish_cmd_usage, 0, NULL);
	return 0;
}
