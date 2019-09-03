#include <string.h>

#include "parse-options.h"

static const struct usage_string channel_cmd_usage[] = {
		USAGE("git chat channel [-l | --list] [-r | --remotes] [-a | --all]"),
		USAGE("git chat channel [(-c | --create)] <name>"),
		USAGE("git chat channel (-s | --switch) <name>"),
		USAGE("git chat channel (-d | --delete) <name>"),
		USAGE("git chat channel --gpg-import-key"),
		USAGE_END()
};

int cmd_channel(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	show_usage(channel_cmd_usage, 0, NULL);
	return 0;
}
