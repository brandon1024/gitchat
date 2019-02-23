#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utils.h>

#include "run-command.h"
#include "usage.h"

#ifndef DEFAULT_GIT_CHAT_TEMPLATES_DIR
#define DEFAULT_GIT_CHAT_TEMPLATES_DIR "/usr/local/share/git-chat/templates"
#endif //DEFAULT_GIT_CHAT_TEMPLATES_DIR

static const struct usage_description init_cmd_usage[] = {
		USAGE("git chat init [(-r | --name) <name>] [(-d | --description) <desc>]"),
		USAGE("git chat init [-q | --quiet] [--rc <config>]"),
		USAGE("git chat init (-h | --help)"),
		USAGE_END()
};

static const struct option_description init_cmd_options[] = {
		OPT_STRING('r', "name", "name", "Specify a name for the room"),
		OPT_STRING('d', "description", "desc", "Specify a room description"),
		OPT_LONG_STRING("rc", "config", "Specify a .roomconfig to use instead of default one"),
		OPT_BOOL('q', "quiet", "Only print error and warning messages"),
		OPT_BOOL('h', "help", "Show usage and exit"),
		OPT_END()
};

/* Function Prototypes */
static int init(char *rc_path, char *room_name, char *room_desc, int quiet);
static void copy_templates_dir(char *templates_dir);
static char *get_cwd(void);
static void safe_create_dir(char *base_path, char *dir);
static void show_init_usage(int err, const char *optional_message_format, ...);

/* Public Functions */
int cmd_init(int argc, char *argv[])
{
	char *rc_path = NULL;
	char *room_name = NULL;
	char *room_desc = NULL;
	bool opt_quiet = false;

	for(size_t arg_index = 0; arg_index < argc; arg_index++) {
		char *arg = argv[arg_index];

		if(!is_valid_argument(arg, init_cmd_options)) {
			show_init_usage(1, "error: unknown flag '%s'", arg);
			return 1;
		}

		//room name
		if(argument_matches_option(arg, init_cmd_options[0])) {
			room_name = argv[arg_index];
			continue;
		}

		//room description
		if(argument_matches_option(arg, init_cmd_options[1])) {
			room_desc = argv[arg_index];
			continue;
		}

		//alternate room config
		if(argument_matches_option(arg, init_cmd_options[2])) {
			rc_path = argv[arg_index];
			continue;
		}

		//quiet
		if(argument_matches_option(arg, init_cmd_options[3])) {
			opt_quiet = true;
			continue;
		}

		//show help
		if(argument_matches_option(arg, init_cmd_options[4])) {
			show_init_usage(0, NULL);
			return 0;
		}
	}

	return init(rc_path, room_name, room_desc, opt_quiet);
}

/* Internal Functions */
static int init(char *rc_path, char *room_name, char *room_desc, int quiet)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);

	cmd.git_cmd = 1;
	cmd.no_out = quiet ? 1 : 0;
	cmd.no_err = quiet ? 1 : 0;
	cmd.no_in = 1;

	argv_array_push(&cmd.args, "init", NULL);
	if(run_command(&cmd))
		FATAL("Failed to 'git init' from the current directory.");

	child_process_def_release(&cmd);

	// create .keys and .cache dirs
	char *cwd = get_cwd();
	safe_create_dir(cwd, ".keys");
	safe_create_dir(cwd, ".cache");
	free(cwd);

	copy_templates_dir(DEFAULT_GIT_CHAT_TEMPLATES_DIR);
	//todo copy roomconfig
	//todo set room name and description

	return 0;
}

static void copy_templates_dir(char *templates_dir)
{
	char *cwd = get_cwd();

	//todo

	free(cwd);
}

static char *get_cwd(void)
{
	char cwd[PATH_MAX];
	if (!getcwd(cwd, PATH_MAX))
		FATAL("Unable to obtain the current working directory from getcwd().");

	struct strbuf buff;
	strbuf_init(&buff);
	strbuf_attach(&buff, cwd, PATH_MAX);

	return strbuf_detach(&buff);
}

static void safe_create_dir(char *base_path, char *dir)
{
	struct strbuf buff;
	strbuf_init(&buff);

	strbuf_attach(&buff, base_path, PATH_MAX);
	strbuf_attach_chr(&buff, '/');
	strbuf_attach_str(&buff, dir);

	if (mkdir(buff.buff, 0777) < 0) {
		if(errno != EEXIST)
			FATAL("Unable to create directory '%s'.", buff.buff);

		LOG_WARN("Directory '%s' already exists.", buff.buff);
	}

	strbuf_release(&buff);
}

static void show_init_usage(int err, const char *optional_message_format, ...)
{
	va_list varargs;
	va_start(varargs, optional_message_format);

	variadic_show_usage_with_options(init_cmd_usage, init_cmd_options,
			optional_message_format, varargs, err);

	va_end(varargs);
}
