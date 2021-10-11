#include "parse-options.h"

static const struct usage_string get_cmd_usage[] = {
		USAGE("git chat get"),
		USAGE_END()
};

int cmd_get(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	show_usage(get_cmd_usage, 0, NULL);
	return 0;
}
