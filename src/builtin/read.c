#include <stdio.h>

#include "parse-options.h"

static const struct usage_string read_cmd_usage[] = {
		USAGE("git chat read [<options>] (<commit-hash> | <filename>)"),
		USAGE("git chat read [(-n | --max-count) <number>] [--dir (ASC | DSC)] [--oneline] [--short]"),
		USAGE("git chat read [--from <email>] [--to <email>]"),
		USAGE("git chat read [--pull-new]"),
		USAGE_END()
};

int cmd_read(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	show_usage(read_cmd_usage, 0, NULL);
	return 0;
}
