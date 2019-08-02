#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "builtin.h"
#include "run-command.h"
#include "fs-utils.h"
#include "usage.h"
#include "version.h"
#include "utils.h"

static const struct usage_description main_cmd_usage[] = {
		USAGE("git chat <command> [<options>]"),
		USAGE("git chat [-h | --help]"),
		USAGE("git chat (-v | --version)"),
		USAGE_END()
};

static const struct option_description main_cmd_options[] = {
		OPT_CMD("channel", "Create and manage communication channels"),
		OPT_CMD("message", "Create messages"),
		OPT_CMD("publish", "Publish messages to the remote server"),
		OPT_CMD("get", "Download messages"),
		OPT_CMD("read", "Display, format and read messages"),
		OPT_BOOL('h', "help", "Show usage and exit"),
		OPT_BOOL('v', "version", "Output version information and exit"),
		OPT_END()
};

static struct cmd_builtin builtins[] = {
		{ "channel", cmd_channel },
		{ "init", cmd_init },
		{ "message", cmd_message },
		{ "publish", cmd_publish },
		{ "get", cmd_get },
		{ "read", cmd_read },
		{ NULL, NULL }
};

static struct cmd_builtin *get_builtin(const char *s);
static int run_builtin(struct cmd_builtin *builtin, int argc, char *argv[]);
static int run_extension(const char *s, int argc, char *argv[]);
static char *find_extension(char *extension_name);
static void show_main_usage(int err, const char *optional_message_format, ...);
static void show_version(void);

int main(int argc, char *argv[])
{
	//Show usage and return if no arguments provided
	if (argc < 2) {
		show_main_usage(0, NULL);
		return 0;
	}

	//Show usage and return if -h argument
	if (argument_matches_option(argv[1], main_cmd_options[5])) {
		show_main_usage(0, NULL);
		return 0;
	}

	//Show version and return if -v argument
	if (argument_matches_option(argv[1], main_cmd_options[6])) {
		show_version();
		return 0;
	}

	//delegate commands
	struct cmd_builtin *builtin = get_builtin(argv[1]);
	if (builtin == NULL) {
		//check if any extensions exist
		char *extension_on_path = find_extension(argv[1]);
		if (extension_on_path) {
			LOG_INFO("builtin: extension '%s' found on the PATH", extension_on_path);

			int status = run_extension(extension_on_path, argc - 2, argv + 2);
			free(extension_on_path);
			return status;
		}

		show_main_usage(1, "error: unknown command or option '%s'", argv[1]);
		return 1;
	}

	return run_builtin(builtin, argc - 2, argv + 2);
}

static struct cmd_builtin *get_builtin(const char *s)
{
	struct cmd_builtin *builtin = builtins;
	while (builtin->cmd != NULL) {
		if (!strcmp(s, builtin->cmd))
			return builtin;

		builtin++;
	}

	return NULL;
}

static int run_builtin(struct cmd_builtin *builtin, int argc, char *argv[])
{
	LOG_INFO("builtin: executing %s builtin", builtin->cmd);
	return builtin->fn(argc, argv);
}

static int run_extension(const char *s, int argc, char *argv[])
{
	LOG_INFO("builtin: executing %s as an extension", s);

	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.executable = s;
	for (size_t i = 0; i < argc; i++)
		argv_array_push(&cmd.args, argv[i], NULL);

	int status = run_command(&cmd);
	child_process_def_release(&cmd);

	return status;
}

static char *find_extension(char *extension_name)
{
	struct strbuf filename;
	strbuf_init(&filename);

	strbuf_attach_str(&filename, "git-chat-");
	strbuf_attach_str(&filename, extension_name);
	char *path = find_in_path(filename.buff);
	strbuf_release(&filename);

	if (!path) {
		return NULL;
	}

	if (!is_executable(path)) {
		return NULL;
	}

	return path;
}

static void show_main_usage(int err, const char *optional_message_format, ...)
{
	va_list varargs;
	va_start(varargs, optional_message_format);

	variadic_show_usage_with_options(main_cmd_usage, main_cmd_options,
			optional_message_format, varargs, err);

	va_end(varargs);
}

static void show_version(void)
{
	fprintf(stdout, "git-chat version %s.%s\n", GITCHAT_VERSION_MAJOR,
			GITCHAT_VERSION_MINOR);

	struct child_process_def cmd;
	child_process_def_init(&cmd);

	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "--version", NULL);
	run_command(&cmd);
	child_process_def_release(&cmd);
}
