#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include "run-command.h"
#include "usage.h"
#include "utils.h"

#define LIST_MODE_LOCAL 0x1
#define LIST_MODE_REMOTE 0x2
#define ACTION_MODE_CREATE 0x1
#define ACTION_MODE_SWITCH 0x2
#define ACTION_MODE_DELETE 0x4
#define ACTION_MODE_GPGIMPORT 0x8

static const struct usage_description channel_cmd_usage[] = {
		USAGE("git chat channel [-l | --list] [-r | --remotes] [-a | --all]"),
		USAGE("git chat channel [(-c | --create)] <name>"),
		USAGE("git chat channel (-s | --switch) <name>"),
		USAGE("git chat channel (-d | --delete) <name>"),
		USAGE("git chat channel --gpg-import-key"),
		USAGE_END()
};

static const struct option_description channel_cmd_options[] = {
		OPT_BOOL('l', "list", "list channel names"),
		OPT_BOOL('r', "remotes", "list remote-tracking channels"),
		OPT_BOOL('a', "all", "list all local and remote-tracking channels"),
		OPT_STRING('c', "create", "name", "create a new channel with the specified name"),
		OPT_STRING('s', "switch", "name", "switch to an existing channel"),
		OPT_STRING('d', "delete", "name", "delete a channel locally"),
		OPT_LONG_BOOL("gpg-import-key", "import public gpg key into channel"),
		OPT_BOOL('h', "help", "Show usage and exit"),
		OPT_END()
};

/* Function Prototypes */
static int list_channels(unsigned char scope);
static int import_gpg_key_to_channel(void);
static int create_channel(const char *channel_name);
static int switch_to_channel(const char *channel_name);
static int delete_channel(const char *channel_name);
static void show_channel_usage(int err, const char *optional_message_format, ...);

/* Public Functions */
int cmd_channel(int argc, char *argv[])
{
	unsigned char list_mode = 0;
	unsigned char action_mode = 0;
	const char *target = NULL;

	if (argc == 0) {
		show_channel_usage(0, NULL);
		return 0;
	}

	//parse arguments
	for (int arg_index = 0; arg_index < argc; arg_index++) {
		size_t arg_char_len = strlen(argv[arg_index]);
		char *arg = argv[arg_index];

		if (!is_valid_argument(arg, channel_cmd_options)) {
			show_channel_usage(1, "error: unknown flag '%s'", arg);
			return 1;
		}

		//handle implicit channel create
		if (arg_char_len > 1 && arg[0] != '-') {
			//if already implicitly defined
			if (action_mode & ACTION_MODE_CREATE) {
				show_channel_usage(1, "error: unknown flag '%s'", arg);
				return 1;
			}

			action_mode |= ACTION_MODE_CREATE;
			target = arg;
			continue;
		}

		//list local channels
		if (argument_matches_option(arg, channel_cmd_options[0]))
			list_mode |= LIST_MODE_LOCAL;

		//list remote channels
		if (argument_matches_option(arg, channel_cmd_options[1]))
			list_mode |= LIST_MODE_REMOTE;

		//list remote channels
		if (argument_matches_option(arg, channel_cmd_options[2]))
			list_mode |= LIST_MODE_LOCAL | LIST_MODE_REMOTE;

		//explicit create branch
		if (argument_matches_option(arg, channel_cmd_options[3])) {
			arg_index++;
			if ((argc-1) < arg_index) {
				show_channel_usage(1, "error: invalid usage of %s. no channel "
						"name specified.", arg);
				return 1;
			}

			action_mode |= ACTION_MODE_CREATE;
			target = argv[arg_index];
		}

		//switch to another channel
		if (argument_matches_option(arg, channel_cmd_options[4])) {
			arg_index++;
			if ((argc-1) < arg_index) {
				show_channel_usage(1, "error: invalid usage of %s. no channel "
						"name specified.", arg);
				return 1;
			}

			action_mode |= ACTION_MODE_SWITCH;
			target = argv[arg_index];
		}

		//delete a channel
		if (argument_matches_option(arg, channel_cmd_options[5])) {
			arg_index++;
			if ((argc-1) < arg_index) {
				show_channel_usage(1, "error: invalid usage of %s. no channel "
						"name specified.", arg);
				return 1;
			}

			action_mode |= ACTION_MODE_DELETE;
			target = argv[arg_index];
		}

		//import gpg key to channel
		if (argument_matches_option(arg, channel_cmd_options[6]))
			action_mode |= ACTION_MODE_GPGIMPORT;

		//show help
		if(argument_matches_option(arg, channel_cmd_options[7])) {
			show_channel_usage(0, NULL);
			return 0;
		}
	}

	//is list mode
	if (list_mode)
		return list_channels(list_mode);

	//is create mode
	if (action_mode == ACTION_MODE_CREATE)
		return create_channel(target);

	//is switch mode
	if (action_mode == ACTION_MODE_SWITCH)
		return switch_to_channel(target);

	//is delete mode
	if (action_mode == ACTION_MODE_DELETE)
		return delete_channel(target);

	//is import key mode
	if (action_mode == ACTION_MODE_GPGIMPORT)
		return import_gpg_key_to_channel();

	//show error if multiple conflicting actions specified
	show_channel_usage(1, "error: invalid argument sequence.");
	return 1;
}

/* Internal Functions */
static int list_channels(unsigned char scope)
{
	struct child_process_def cmd;
	int ret = 0;

	child_process_def_init(&cmd);
	cmd.git_cmd = 1;

	if (scope == LIST_MODE_LOCAL)
		argv_array_push(&cmd.args, "branch", "-l", NULL);
	else if (scope == LIST_MODE_REMOTE)
		argv_array_push(&cmd.args, "branch", "-r", NULL);
	else if (scope == (LIST_MODE_LOCAL | LIST_MODE_REMOTE))
		argv_array_push(&cmd.args, "branch", "-a", NULL);
	else
		FATAL("invalid scope '%x'", scope);

	ret = run_command(&cmd);
	child_process_def_release(&cmd);

	return ret;
}

static int import_gpg_key_to_channel(void)
{
	return 0;
}

static int create_channel(const char *channel_name)
{
	struct child_process_def cmd;
	int ret = 0;

	child_process_def_init(&cmd);
	cmd.git_cmd = 1;

	argv_array_push(&cmd.args, "checkout", "-b", channel_name, NULL);
	ret = run_command(&cmd);
	child_process_def_release(&cmd);

	return ret;
}

static int switch_to_channel(const char *channel_name)
{
	struct child_process_def cmd;
	int ret = 0;

	child_process_def_init(&cmd);
	cmd.git_cmd = 1;

	argv_array_push(&cmd.args, "checkout", channel_name, NULL);
	ret = run_command(&cmd);
	child_process_def_release(&cmd);

	return ret;
}

static int delete_channel(const char *channel_name)
{
	struct child_process_def cmd;
	int ret = 0;

	child_process_def_init(&cmd);
	cmd.git_cmd = 1;

	argv_array_push(&cmd.args, "checkout", "-d", channel_name, NULL);
	ret = run_command(&cmd);
	child_process_def_release(&cmd);

	return ret;
}

static void show_channel_usage(int err, const char *optional_message_format, ...)
{
	va_list varargs;
	va_start(varargs, optional_message_format);

	variadic_show_usage_with_options(channel_cmd_usage, channel_cmd_options,
			optional_message_format, varargs, err);

	va_end(varargs);
}
