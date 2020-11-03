#include <stdio.h>

#include "parse-options.h"
#include "run-command.h"
#include "utils.h"
#include "working-tree.h"

static const struct usage_string channel_cmd_usage[] = {
		USAGE("git chat channel delete <ref>"),
		USAGE_END()
};

int channel_delete(int argc, char *argv[])
{
	int show_help = 0;

	const struct command_option channel_cmd_options[] = {
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	argc = parse_options(argc, argv, channel_cmd_options, 0, 1);
	if (show_help) {
		show_usage_with_options(channel_cmd_usage, channel_cmd_options, 0, NULL);
		return 0;
	}

	if (argc != 1) {
		const char *err_msg = !argc ? "error: too few options" : "error: too many options";
		show_usage_with_options(channel_cmd_usage, channel_cmd_options, 1, err_msg);
		return 1;
	}

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	const char *channel_ref = argv[0];

	struct child_process_def cmd;
	child_process_def_init(&cmd);

	cmd.git_cmd = 1;
	cmd.std_fd_info = STDOUT_NULL | STDERR_NULL | STDIN_NULL;
	argv_array_push(&cmd.args, "branch", "-D", channel_ref, NULL);

	int status = run_command(&cmd);
	if (status)
		DIE("couldn't delete channel '%s'", channel_ref);

	child_process_def_release(&cmd);

	printf("Deleted channel '%s'\n", channel_ref);

	return 0;
}

