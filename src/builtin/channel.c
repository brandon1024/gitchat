#include <string.h>

#include "parse-options.h"

static const struct usage_string channel_cmd_usage[] = {
		USAGE("git chat channel (create | new) [(-n | --name) <alias>] [(-d | --description) <description>] <ref>"),
		USAGE("git chat channel (switch | sw) <ref>"),
		USAGE("git chat channel (delete | rm) <ref>"),
		USAGE("git chat channel (list | ls) [(-a | --all)]"),
		USAGE("git chat channel [<subcommand>] [(-h | --help)]"),
		USAGE_END()
};

extern int channel_create(int argc, char *argv[]);
extern int channel_switch(int argc, char *argv[]);
extern int channel_delete(int argc, char *argv[]);
extern int channel_list(int argc, char *argv[]);

int cmd_channel(int argc, char *argv[])
{
	int show_help = 0;

	const struct command_option channel_cmd_options[] = {
			OPT_CMD("create", "create a new channel", NULL),
			OPT_CMD("switch", "switch to another channel", NULL),
			OPT_CMD("delete", "delete a channel", NULL),
			OPT_CMD("list", "list channels", NULL),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	argc = parse_options(argc, argv, channel_cmd_options, 1, 1);
	if (!argc || show_help) {
		show_usage_with_options(channel_cmd_usage, channel_cmd_options, 0, NULL);
		return 0;
	}

	if (!strcmp(argv[0], "create") || !strcmp(argv[0], "new"))
		return channel_create(argc - 1, argv + 1);
	if (!strcmp(argv[0], "switch") || !strcmp(argv[0], "sw"))
		return channel_switch(argc - 1, argv + 1);
	if (!strcmp(argv[0], "delete") || !strcmp(argv[0], "rm"))
		return channel_delete(argc - 1, argv + 1);
	if (!strcmp(argv[0], "list") || !strcmp(argv[0], "ls"))
		return channel_list(argc - 1, argv + 1);

	show_usage_with_options(channel_cmd_usage, channel_cmd_options, 1, "error: unknown subcommand '%s'", argv[0]);
	return 1;
}
