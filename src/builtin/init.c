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
		OPT_STRING('n', "name", "name", "specify a name for the master channel"),
		OPT_STRING('d', "description", "desc", "specify a description for the space"),
		OPT_BOOL('q', "quiet", "only print error and warning messages"),
		OPT_BOOL('h', "help", "show usage and exit"),
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
			show_init_usage(1, "error: unknown option '%s'", arg);
			return 1;
		}

		//room name
		if (argument_matches_option(arg, init_cmd_options[0])) {
			if (++arg_index >= argc) {
				show_init_usage(1, "error: no channel name provided with %s", arg);
				return 1;
			}

			channel_name = argv[arg_index];
			continue;
		}

		//room description
		if (argument_matches_option(arg, init_cmd_options[1])) {
			if (++arg_index >= argc) {
				show_init_usage(1, "error: no room description provided with %s", arg);
				return 1;
			}

			room_desc = argv[arg_index];
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

	if (is_inside_git_chat_space())
		DIE("A git-chat space cannot be reinitialized.");

	LOG_INFO("Initializing new space '%s' with master channel name '%s'",
			space_desc, channel_name);

	struct strbuf templates_dir_path;
	strbuf_init(&templates_dir_path);

	strbuf_attach_str(&templates_dir_path, DEFAULT_GIT_CHAT_TEMPLATES_DIR);
	struct stat sb;
	if (stat(templates_dir_path.buff, &sb) == -1) {
		if (errno == EACCES)
			DIE("Something's not quite right with your installation.\n"
				"Tried to copy from template directory '%s' but don't have "
				"sufficient permission to read from there.",
				DEFAULT_GIT_CHAT_TEMPLATES_DIR);

		LOG_INFO("Could not stat '%s'. Falling back on $HOME/share/git-chat/templates",
				DEFAULT_GIT_CHAT_TEMPLATES_DIR);

		strbuf_release(&templates_dir_path);
		const char *home = getenv("HOME");
		if (!home || !*home)
			DIE("Tried to copy from template directory HOME/share/git-chat/templates, "
				"but the $HOME environment variable is not set.");

		strbuf_init(&templates_dir_path);
		strbuf_attach_fmt(&templates_dir_path, "%s/share/git-chat/templates", home);
		if (stat(templates_dir_path.buff, &sb) == -1) {
			if (errno == EACCES)
				DIE("Something's not quite right with your installation.\n"
					"Tried to copy from template directory '%s' but don't have "
					"sufficient permission to read from there.",
					templates_dir_path.buff);

			DIE("Something's not quite right with your installation.\n"
				"Tried to copy from template directory but directory doesn't exist.\n"
				"Looked in the following places:\n"
				"%s\n"
				"%s",
				DEFAULT_GIT_CHAT_TEMPLATES_DIR,
				templates_dir_path.buff);
		}
	}

	cmd.git_cmd = 1;
	child_process_def_stdin(&cmd, STDIN_NULL);
	if (quiet) {
		child_process_def_stdout(&cmd, STDOUT_NULL);
		child_process_def_stderr(&cmd, STDERR_NULL);
	}

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
	safe_create_dir(cwd_path_buf.buff, ".keys", 0777);
	LOG_INFO("Created .keys directory");

	// create .keys directory
	safe_create_dir(cwd_path_buf.buff, ".git/chat-cache", 0777);
	LOG_INFO("Created .git/chat-cache directory");

	//recursively copy from template dir into .git-chat
	struct strbuf git_chat_path;
	strbuf_init(&git_chat_path);
	strbuf_attach_fmt(&git_chat_path, "%s/.git-chat", cwd_path_buf.buff);
	copy_dir(templates_dir_path.buff, git_chat_path.buff);
	LOG_INFO("Copied directory from '%s' to '%s'", templates_dir_path.buff,
			git_chat_path.buff);

	struct strbuf author;
	strbuf_init(&author);

	char *author_str = NULL;
	if (!get_author_identity(&author)) {
		author_str = author.buff;
		LOG_INFO("Found user details (%s) from global .gitconfig", author_str);
	} else
		WARN("Unable to retrieve your user information. Is your user configured through .gitconfig?");

	update_config(git_chat_path.buff, channel_name, author_str);
	update_space_description(git_chat_path.buff, space_desc);

	initialize_channel_root(git_chat_path.buff);

	if (!quiet)
		fprintf(stdout, "Successfully initialized git-chat space.\n");

	strbuf_release(&templates_dir_path);
	strbuf_release(&author);
	strbuf_release(&cwd_path_buf);
	strbuf_release(&git_chat_path);

	return 0;
}

/**
 * Update the channel name and channel creator in .git-chat/config.
 *
 * The argument 'base' is a full path to the .git-chat directory.
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

	LOG_INFO("Updated master channel configuration");
}

/**
 * Update the description of the space by writing to .git-chat/description.
 *
 * The argument 'base' is a full path to the .git-chat directory.
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
	if (recoverable_write(desc_fd, description, len) != len)
		FATAL("failed to write to description file file '%s'", desc_path);

	close(desc_fd);
	strbuf_release(&desc_path);

	LOG_INFO("Updated space description to '%s'", description);
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
	cmd.std_fd_info = STDIN_NULL | STDOUT_NULL | STDERR_NULL;

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
	cmd.std_fd_info = STDIN_NULL | STDOUT_NULL | STDERR_NULL;

	argv_array_push(&cmd.args, "commit", "--no-gpg-sign", "--no-verify",
			"--message", "You have reached the beginning of time.", NULL);

	ret = run_command(&cmd);
	if (ret)
		DIE("unable to create initial commit; git exited with status %d", ret);

	child_process_def_release(&cmd);

	LOG_INFO("Successfully created channel root commit");
}

/**
 * Attempt to fetch the user identify from their .gitconfig. The author's name
 * is chosen, in the following order:
 * 1. user.username
 * 2. user.email
 * 3. user.name
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
	argv_array_push(&cmd.args, "config", "--get", "user.username", NULL);
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
	argv_array_push(&cmd.args, "config", "--get", "user.email", NULL);
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
	argv_array_push(&cmd.args, "config", "--get", "user.name", NULL);
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
