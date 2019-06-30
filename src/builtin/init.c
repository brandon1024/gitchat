#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

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
static int init(const char *channel_name, const char *space_desc, int quiet);
static void update_config(char *base, const char *channel_name, const char *author);
static void update_space_description(char *base, const char *description);
static void initialize_channel_root(char *base);
int get_author_identity(struct strbuf *result);
static void show_init_usage(int err, const char *optional_message_format, ...);

/* Public Functions */
int cmd_init(int argc, char *argv[])
{
	char *channel_name = NULL;
	char *room_desc = NULL;
	int opt_quiet = 0;

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
			opt_quiet = 1;
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

static int init(const char *channel_name, const char *space_desc, const int quiet)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);

	LOG_INFO("Initializing new space '%s' with master channel name '%s'",
			space_desc, channel_name);

	struct stat sb;
	if (stat(DEFAULT_GIT_CHAT_TEMPLATES_DIR, &sb) == -1) {
		if (errno == EACCES)
			DIE("Something's not quite right with your installation.\n"
				"Tried to copy from template directory '%s' but don't have\n"
				"sufficient permission to read from there.",
				DEFAULT_GIT_CHAT_TEMPLATES_DIR);

		DIE("Something's not quite right with your installation.\n"
			"Tried to copy from template directory '%s' but directory doesn't exist.",
			DEFAULT_GIT_CHAT_TEMPLATES_DIR);
	}

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
	strbuf_attach_fmt(&cache_path, "%s/.cache", cwd_path_buf.buff);
	copy_dir(DEFAULT_GIT_CHAT_TEMPLATES_DIR, cache_path.buff);
	LOG_INFO("Copied directory from " DEFAULT_GIT_CHAT_TEMPLATES_DIR " to '%s'",
			 cache_path.buff);

	struct strbuf author;
	strbuf_init(&author);

	char *author_str = NULL;
	if (!get_author_identity(&author))
		author_str = author.buff;
	else
		WARN("Unable to retrieve your user information. Is your user configured through .gitconfig?");

	update_config(cache_path.buff, channel_name, author_str);
	update_space_description(cache_path.buff, space_desc);

	initialize_channel_root(cache_path.buff);

	if (!quiet)
		fprintf(stdout, "Successfully initialized git-chat space.\n");

	strbuf_release(&author);
	strbuf_release(&cwd_path_buf);
	strbuf_release(&cache_path);

	return 0;
}

/**
 * Update the channel name and channel creator in .cache/config.
 *
 * The argument 'base' is a full path to the .cache directory.
 * */
static void update_config(char *base, const char *channel_name, const char *author)
{
	struct strbuf config_path;
	strbuf_init(&config_path);
	strbuf_attach_fmt(&config_path, "%s/config", base);

	struct conf_data conf;
	int ret = parse_config(&conf, config_path.buff);
	if (ret < 0)
		DIE("unable to update '%s'; cannot access file", config_path);
	else if (ret > 0)
		DIE("unable to update '%s'; file contains syntax errors", config_path);

	if (channel_name) {
		struct conf_data_entry *entry = conf_data_find_entry(&conf, "channel.master", "name");
		if (!entry)
			DIE("unexpected config template with missing key 'channel.master.name'");

		//overwrite updated config file
		free(entry->value);
		entry->value = strdup(channel_name);
		if (!entry->value)
			FATAL(MEM_ALLOC_FAILED);
	}

	if (author) {
		struct conf_data_entry *entry = conf_data_find_entry(&conf, "channel.master", "createdby");
		if (!entry)
			DIE("unexpected config template with missing key 'channel.master.createdby'");

		//overwrite updated config file
		free(entry->value);
		entry->value = strdup(author);
		if (!entry->value)
			FATAL(MEM_ALLOC_FAILED);
	}

	write_config(&conf, config_path.buff);
	strbuf_release(&config_path);
	release_config_resources(&conf);
}

/**
 * Update the description of the space by writing to .cache/description.
 *
 * The argument 'base' is a full path to the .cache directory.
 * */
static void update_space_description(char *base, const char *description)
{
	struct strbuf desc_path;

	if (!description)
		return;

	strbuf_init(&desc_path);
	strbuf_attach_fmt(&desc_path, "%s/description", base);

	int desc_fd = open(desc_path.buff, O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	if (desc_fd < 0)
		FATAL(FILE_OPEN_FAILED, desc_path);

	size_t len = strlen(description);
	if (write(desc_fd, description, len) != len)
		FATAL("failed to write to description file file '%s'", desc_path);

	close(desc_fd);
	strbuf_release(&desc_path);
}

/**
 * Initialize the master channel by making the first commit on the branch. The first
 * commit will have only two tracked files, config and description. The commit
 * message will have the following message:
 *
 * "You have reached the beginning of time."
 *
 * Note that this commit will not be gpg signed. Also, the '--no-verify' flag is
 * used to suppress any hooks that the user may have configured.
 * */
static void initialize_channel_root(char *base)
{
	//Add config and description to index
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;
	cmd.no_in = 1;

	struct strbuf config_path;
	strbuf_init(&config_path);
	strbuf_attach_fmt(&config_path, "%s/config", base);

	struct strbuf description_path;
	strbuf_init(&description_path);
	strbuf_attach_fmt(&description_path, "%s/description", base);

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

/**
 * Attempt to fetch the user identify from their .gitconfig. The author's name
 * is chosen, in the following order:
 * 1. user.username
 * 2. user.name
 * 3. user.email
 *
 * If none of these are available (i.e. git returns a status of 1 for all three),
 * then this function returns 1. Otherwise, returns 0 and populates the given
 * strbuf with the author's name.
 * */
int get_author_identity(struct strbuf *result)
{
	int ret = 0;
	struct strbuf cmd_out;
	strbuf_init(&cmd_out);

	//prefer user.username
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.no_in = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;
	argv_array_push(&cmd.args, "config", "--get", "user.username", NULL);
	ret = capture_command(&cmd, &cmd_out);
	child_process_def_release(&cmd);
	if (!ret) {
		strbuf_trim(&cmd_out);
		strbuf_attach(result, cmd_out.buff, cmd_out.len);
		strbuf_release(&cmd_out);
		return 0;
	}

	//then user.name
	strbuf_release(&cmd_out);
	strbuf_init(&cmd_out);
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.no_in = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;
	argv_array_push(&cmd.args, "config", "--get", "user.name", NULL);
	ret = capture_command(&cmd, &cmd_out);
	child_process_def_release(&cmd);
	if (!ret) {
		strbuf_trim(&cmd_out);
		strbuf_attach(result, cmd_out.buff, cmd_out.len);
		strbuf_release(&cmd_out);
		return 0;
	}

	//then user.email
	strbuf_release(&cmd_out);
	strbuf_init(&cmd_out);
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.no_in = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;
	argv_array_push(&cmd.args, "config", "--get", "user.email", NULL);
	ret = capture_command(&cmd, &cmd_out);
	child_process_def_release(&cmd);
	if (!ret) {
		strbuf_trim(&cmd_out);
		strbuf_attach(result, cmd_out.buff, cmd_out.len);
		strbuf_release(&cmd_out);
		return 0;
	}

	strbuf_release(&cmd_out);

	return 1;
}

static void show_init_usage(int err, const char *optional_message_format, ...)
{
	va_list varargs;
	va_start(varargs, optional_message_format);

	variadic_show_usage_with_options(init_cmd_usage, init_cmd_options,
			optional_message_format, varargs, err);

	va_end(varargs);
}
