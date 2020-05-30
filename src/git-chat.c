#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtin.h"
#include "run-command.h"
#include "fs-utils.h"
#include "gpg-common.h"
#include "version.h"
#include "parse-options.h"
#include "utils.h"

static const struct usage_string main_cmd_usage[] = {
		USAGE("git chat <command> [<options>]"),
		USAGE("git chat [-h | --help]"),
		USAGE("git chat (-v | --version)"),
		USAGE_END()
};

static struct cmd_builtin *get_builtin(const char *);
static int run_builtin(struct cmd_builtin *, int, char *[]);
static int run_extension(const char *, int, char *[]);
static char *find_extension(char *);
static void print_version(void);

int main(int argc, char *argv[])
{
	int show_help = 0;
	int show_version = 0;

	const struct command_option main_cmd_options[] = {
			OPT_CMD("channel", "Create and manage communication channels", NULL),
			OPT_CMD("message", "Create messages", NULL),
			OPT_CMD("publish", "Publish messages to the remote server", NULL),
			OPT_CMD("get", "Download messages", NULL),
			OPT_CMD("read", "Display, format and read messages", NULL),
			OPT_CMD("import-key", "Import a GPG key into the current channel", NULL),
			OPT_BOOL('h', "help", "Show usage and exit", &show_help),
			OPT_BOOL('v', "version", "Output version information and exit", &show_version),
			OPT_END()
	};

	// show usage and return if no arguments provided
	if (argc < 2) {
		show_usage_with_options(main_cmd_usage, main_cmd_options, 0, NULL);
		return 0;
	}

	argc = parse_options(argc, argv, main_cmd_options, 1, 1);
	if (show_help) {
		show_usage_with_options(main_cmd_usage, main_cmd_options, 0, NULL);
		return 0;
	}

	if (show_version) {
		print_version();
		return 0;
	}

	//delegate commands
	struct cmd_builtin *builtin = get_builtin(argv[0]);
	if (builtin == NULL) {
		//check if any extensions exist
		char *extension_on_path = find_extension(argv[0]);
		if (extension_on_path) {
			LOG_INFO("builtin: extension '%s' found on the PATH", extension_on_path);

			int status = run_extension(extension_on_path, argc - 1, argv + 1);
			free(extension_on_path);
			return status;
		}

		show_usage_with_options(main_cmd_usage, main_cmd_options, 1, "error: unknown command or option '%s'", argv[0]);
		return 1;
	}

	return run_builtin(builtin, argc, argv);
}

static struct cmd_builtin *get_builtin(const char *s)
{
	struct cmd_builtin *builtin = registered_builtins;
	while (builtin->cmd != NULL) {
		if (!strcmp(s, builtin->cmd))
			return builtin;

		builtin++;
	}

	return NULL;
}

static int run_builtin(struct cmd_builtin *builtin, int argc, char *argv[])
{
	init_gpgme_openpgp_engine();

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

static void print_version(void)
{
	fprintf(stdout, "git-chat version %s\n", GIT_CHAT_VERSION);

	struct child_process_def cmd;
	child_process_def_init(&cmd);

	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "--version", NULL);
	run_command(&cmd);
	child_process_def_release(&cmd);

	fprintf(stdout, "gpgme library version %s\n",
			get_gpgme_library_version());
}
