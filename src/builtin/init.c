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
#include <dirent.h>
#include <fcntl.h>

#include "run-command.h"
#include "parse-config.h"
#include "usage.h"
#include "fs-utils.h"
#include "utils.h"

#ifndef DEFAULT_GIT_CHAT_TEMPLATES_DIR
#define DEFAULT_GIT_CHAT_TEMPLATES_DIR "/usr/local/share/git-chat/templates"
#endif //DEFAULT_GIT_CHAT_TEMPLATES_DIR

static const struct usage_description init_cmd_usage[] = {
		USAGE("git chat init [(-n | --name) <name>] [(-d | --description) <desc>]"),
		USAGE("git chat init [-q | --quiet]"),
		USAGE("git chat init (-h | --help)"),
		USAGE_END()
};

static const struct option_description init_cmd_options[] = {
		OPT_STRING('n', "name", "name", "Specify a name for the master channel"),
		OPT_STRING('d', "description", "desc", "Specify a description for the space"),
		OPT_BOOL('q', "quiet", "Only print error and warning messages"),
		OPT_BOOL('h', "help", "Show usage and exit"),
		OPT_END()
};

/* Function Prototypes */
static int init(char *channel_name, char *space_desc, int quiet);
static void update_config(char *base, char *channel_name, char *author);
static void update_space_description(char *base, char *description);
static void initialize_channel_root(char *base);
static void show_init_usage(int err, const char *optional_message_format, ...);

/* Public Functions */
int cmd_init(int argc, char *argv[])
{
	char *channel_name = NULL;
	char *room_desc = NULL;
	bool opt_quiet = false;

	for (size_t arg_index = 0; arg_index < argc; arg_index++) {
		char *arg = argv[arg_index];

		if (!is_valid_argument(arg, init_cmd_options)) {
			show_init_usage(1, "error: unknown flag '%s'", arg);
			return 1;
		}

		//room name
		if (argument_matches_option(arg, init_cmd_options[0])) {
			channel_name = argv[++arg_index];
			continue;
		}

		//room description
		if (argument_matches_option(arg, init_cmd_options[1])) {
			room_desc = argv[++arg_index];
			continue;
		}

		//quiet
		if (argument_matches_option(arg, init_cmd_options[2])) {
			opt_quiet = true;
			continue;
		}

		//show help
		if (argument_matches_option(arg, init_cmd_options[3])) {
			show_init_usage(0, NULL);
			return 0;
		}
	}

	return init(channel_name, room_desc, opt_quiet);
}

static int init(char *channel_name, char *space_desc, int quiet)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);

	LOG_INFO("Initializing new space '%s' with master channel name '%s'",
			space_desc, channel_name);

	cmd.git_cmd = 1;
	cmd.no_out = quiet ? 1 : 0;
	cmd.no_err = quiet ? 1 : 0;
	cmd.no_in = 1;

	argv_array_push(&cmd.args, "init", NULL);
	int ret = run_command(&cmd);
	if (ret)
		DIE("unable to 'git init' from the current directory; "
				"git exited with status %d", ret);

	child_process_def_release(&cmd);

	struct strbuf cwd_path_buf;
	strbuf_init(&cwd_path_buf);
	if (get_cwd(&cwd_path_buf))
		FATAL("unable to obtain the current working directory from getcwd()");

	// create .keys directory
	safe_create_dir(cwd_path_buf.buff, ".keys");

	//recursively copy from template dir into .cache
	struct strbuf cache_path;
	strbuf_init(&cache_path);
	strbuf_attach_str(&cache_path, cwd_path_buf.buff);
	strbuf_attach_str(&cache_path, "/.cache");
	copy_dir(DEFAULT_GIT_CHAT_TEMPLATES_DIR, cache_path.buff);
	LOG_INFO("Copied directory from " DEFAULT_GIT_CHAT_TEMPLATES_DIR " to '%s'",
			 cache_path.buff);

	update_config(cache_path.buff, channel_name, NULL);
	update_space_description(cache_path.buff, space_desc);

	initialize_channel_root(cwd_path_buf.buff);

	if (!quiet)
		fprintf(stdout, "Successfully initialized git-chat space.\n");

	strbuf_release(&cwd_path_buf);
	strbuf_release(&cache_path);

	return 0;
}

static void update_config(char *base, char *channel_name, char *author)
{
	struct strbuf config_path;
	strbuf_init(&config_path);
	strbuf_attach_str(&config_path, base);
	strbuf_attach_str(&config_path, "/.config");

	struct conf_data conf;
	int ret = parse_config(&conf, config_path.buff);
	if (ret < 0)
		DIE("unable to update '%s'; cannot access file", config_path);
	else if (ret > 0)
		DIE("unable to update '%s'; file contains syntax errors", config_path);

	if (channel_name) {
		struct conf_data_entry *entry = conf_data_find_entry(&conf, "channel.master", "name");
		if (!entry)
			DIE("unexpected .config template with missing key 'channel.master.name'");

		//overwrite updated config file
		free(entry->value);
		entry->value = strdup(channel_name);
		if (!entry->value)
			FATAL(MEM_ALLOC_FAILED);
	}

	if (author) {
		struct conf_data_entry *entry = conf_data_find_entry(&conf, "channel.master", "createdby");
		if (!entry)
			DIE("unexpected .config template with missing key 'channel.master.createdby'");

		//overwrite updated config file
		free(entry->value);
		entry->value = strdup(channel_name);
		if (!entry->value)
			FATAL(MEM_ALLOC_FAILED);
	}

	write_config(&conf, config_path.buff);
	release_config_resources(&conf);
}

static void update_space_description(char *base, char *description)
{
	struct strbuf desc_path;

	if (!description)
		return;

	strbuf_init(&desc_path);
	strbuf_attach_str(&desc_path, base);
	strbuf_attach_str(&desc_path, "/description");

	int desc_fd = open(desc_path.buff, O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (desc_fd < 0)
		FATAL(FILE_OPEN_FAILED, desc_path);

	size_t len = strlen(description);
	if (write(desc_fd, description, len) != len)
		FATAL("failed to write to description file file '%s'", desc_path);

	close(desc_fd);
}

static void initialize_channel_root(char *base)
{
	//Add .config and description to index
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;
	cmd.no_in = 1;

	struct strbuf config_path;
	strbuf_init(&config_path);
	strbuf_attach_str(&config_path, base);
	strbuf_attach_str(&config_path, "/.config");

	struct strbuf description_path;
	strbuf_init(&description_path);
	strbuf_attach_str(&description_path, base);
	strbuf_attach_str(&description_path, "/description");

	argv_array_push(&cmd.args, "add", config_path.buff, description_path.buff, NULL);
	int ret = run_command(&cmd);
	if (ret)
		DIE("unable to 'git add' from the current directory; git exited with status %d", ret);

	strbuf_release(&config_path);
	strbuf_release(&description_path);
	child_process_def_release(&cmd);

	//create commit object
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;
	cmd.no_in = 1;

	argv_array_push(&cmd.args, "commit", "--no-gpg-sign", "--no-verify",
			"--message", "You have reached the beginning of time.", NULL);

	ret = run_command(&cmd);
	if (ret)
		DIE("unable to create initial commit; git exited with status %d", ret);

	child_process_def_release(&cmd);
}

static void show_init_usage(int err, const char *optional_message_format, ...)
{
	va_list varargs;
	va_start(varargs, optional_message_format);

	variadic_show_usage_with_options(init_cmd_usage, init_cmd_options,
			optional_message_format, varargs, err);

	va_end(varargs);
}
