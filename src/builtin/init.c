#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "run-command.h"
#include "working-tree.h"
#include "parse-options.h"
#include "parse-config.h"
#include "fs-utils.h"
#include "utils.h"

#ifndef DEFAULT_GIT_CHAT_TEMPLATES_DIR
#define DEFAULT_GIT_CHAT_TEMPLATES_DIR "/usr/local/share/git-chat/templates"
#endif //DEFAULT_GIT_CHAT_TEMPLATES_DIR

static const struct usage_string init_cmd_usage[] = {
		USAGE("git chat init [(-n | --name) <name>] [(-d | --description) <desc>]"),
		USAGE("git chat init [-q | --quiet]"),
		USAGE("git chat init (-h | --help)"),
		USAGE_END()
};

static int init(const char *, const char *, int );

int cmd_init(int argc, char *argv[])
{
	char *channel_name = NULL;
	char *room_desc = NULL;
	int opt_quiet = 0;
	int show_help = 0;

	const struct command_option init_cmd_options[] = {
			OPT_STRING('n', "name", "name", "specify a name for the master channel", &channel_name),
			OPT_STRING('d', "description", "desc", "specify a description for the space", &room_desc),
			OPT_BOOL('q', "quiet", "only print error and warning messages", &opt_quiet),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	argc = parse_options(argc, argv, init_cmd_options, 1, 1);
	if (argc > 0) {
		show_usage_with_options(init_cmd_usage, init_cmd_options, 1, "error: unknown option '%s'", argv[0]);
		return 1;
	}

	if (show_help) {
		show_usage_with_options(init_cmd_usage, init_cmd_options, 0, NULL);
		return 0;
	}

	return init(channel_name, room_desc, opt_quiet);
}

static void prepare_git_chat(const char *, const char *, const char *);
static int get_author_identity(struct strbuf *);
static void initialize_channel_root();

static int init(const char *channel_name, const char *space_desc, const int quiet)
{
	if (is_inside_git_chat_space())
		DIE("a git-chat space cannot be reinitialized.");

	LOG_INFO("Initializing new space '%s' with master channel name '%s'",
			 space_desc, channel_name);

	// execute 'git init' as child process
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	child_process_def_stdin(&cmd, STDIN_NULL);
	argv_array_push(&cmd.args, "init", NULL);
	if (quiet) {
		child_process_def_stdout(&cmd, STDOUT_NULL);
		argv_array_push(&cmd.args, "-q", NULL);
	}

	int ret = run_command(&cmd);
	if (ret)
		DIE("'git init' failed from the current directory; "
				"git exited with status %d", ret);

	child_process_def_release(&cmd);

	struct strbuf author;
	strbuf_init(&author);
	if (get_author_identity(&author)) {
		WARN("Unable to retrieve your user git information.\n"
			 "Is your user configured through .gitconfig?");
	}

	LOG_DEBUG("Found user details (%s) from global .gitconfig", author.buff);

	// create .git-chat dir
	prepare_git_chat(channel_name, space_desc, author.buff);

	// create initial commit
	initialize_channel_root();

	if (!quiet)
		fprintf(stdout, "Successfully initialized git-chat space.\n");

	strbuf_release(&author);

	return 0;
}

static void update_config(char *, const char *, const char *);
static void update_space_description(char *, const char *);

/**
 * Initialize git-chat. Performs the following:
 * - copies templates directory to $cwd/.git-chat
 * - updates config and channel description
 * - creates .git-chat/keys directory
 * - creates .git/.chat-cache directory
 *
 * If any of those steps fail, the application will DIE().
 * */
static void prepare_git_chat(const char *channel_name, const char *description, const char *author)
{
	struct strbuf templates_dir_path;
	strbuf_init(&templates_dir_path);

	struct strbuf path;
	strbuf_init(&path);

	strbuf_attach_str(&templates_dir_path, DEFAULT_GIT_CHAT_TEMPLATES_DIR);
	struct stat sb;
	if (stat(templates_dir_path.buff, &sb) == -1) {
		if (errno == EACCES)
			DIE("Something's not quite right with your installation.\n"
				"Tried to copy from template directory '%s' but don't have "
				"sufficient permission to read from there.",
				DEFAULT_GIT_CHAT_TEMPLATES_DIR);

		LOG_WARN("Could not stat default templates directory.\n"
				"Falling back on $HOME/share/git-chat/templates");

		strbuf_release(&templates_dir_path);
		const char *home = getenv("HOME");
		if (!home || !*home)
			DIE("Tried to copy from template directory $HOME/share/git-chat/templates, "
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

	// copy templates directory and update config
	if (get_git_chat_dir(&path))
		FATAL("failed to obtain .git-chat dir");
	copy_dir(templates_dir_path.buff, path.buff);

	LOG_DEBUG("Copied directory from '%s' to '%s'", templates_dir_path.buff,
			  path.buff);

	update_config(path.buff, channel_name, author);
	update_space_description(path.buff, description);

	// create keys directory
	strbuf_clear(&path);
	if (get_keys_dir(&path))
		FATAL("failed to obtain git-chat keys dir");
	
	safe_create_dir(path.buff, NULL, S_IRWXU | S_IRGRP | S_IROTH);
	LOG_DEBUG("Created directory for gpg keys '%s'", path.buff);

	// create chat cache directory
	strbuf_clear(&path);
	if (get_chat_cache_dir(&path))
		FATAL("failed to obtain chat cache dir");

	safe_create_dir(path.buff, NULL, S_IRWXU | S_IRGRP | S_IROTH);
	LOG_DEBUG("Created chat cache directory '%s'", path.buff);

	strbuf_release(&path);
	strbuf_release(&templates_dir_path);
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

	struct config_file_data conf;
	int ret = parse_config(&conf, config_path.buff);
	if (ret < 0)
		DIE("unable to update '%s'; cannot access file", config_path);
	if (ret > 0)
		DIE("unable to update '%s'; file contains syntax errors", config_path);

	if (channel_name) {
		struct config_entry *entry = config_file_data_find_entry(&conf, "channel.master.name");
		if (!entry)
			DIE("unexpected config template with missing key 'channel.master.name'");

		//overwrite updated config file
		config_file_data_set_entry_value(entry, channel_name);
	}

	if (author) {
		struct config_entry *entry = config_file_data_find_entry(&conf, "channel.master.createdby");
		if (!entry)
			DIE("unexpected config template with missing key 'channel.master.createdby'");

		// overwrite updated config file
		config_file_data_set_entry_value(entry, author);
	}

	write_config(&conf, config_path.buff);
	strbuf_release(&config_path);
	config_file_data_release(&conf);

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
static void initialize_channel_root()
{
	// add config and description to index
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.std_fd_info = STDIN_NULL | STDOUT_NULL | STDERR_NULL;

	struct strbuf git_chat_dir;
	strbuf_init(&git_chat_dir);
	if (get_git_chat_dir(&git_chat_dir))
		FATAL("unable to obtain .git-chat dir");

	struct strbuf config_path;
	strbuf_init(&config_path);
	strbuf_attach_fmt(&config_path, "%s/config", git_chat_dir.buff);

	struct strbuf description_path;
	strbuf_init(&description_path);
	strbuf_attach_fmt(&description_path, "%s/description", git_chat_dir.buff);

	argv_array_push(&cmd.args, "add", config_path.buff, description_path.buff, NULL);
	int ret = run_command(&cmd);
	if (ret)
		DIE("unable to 'git add' from the current directory; git exited with status %d", ret);

	strbuf_release(&config_path);
	strbuf_release(&description_path);
	child_process_def_release(&cmd);

	// create commit object
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.std_fd_info = STDIN_NULL | STDOUT_NULL | STDERR_NULL;

	argv_array_push(&cmd.args, "commit", "--no-gpg-sign", "--no-verify",
			"--message", "You have reached the beginning of time.", NULL);

	ret = run_command(&cmd);
	if (ret)
		DIE("unable to create initial commit; git exited with status %d", ret);

	child_process_def_release(&cmd);

	LOG_DEBUG("Successfully created channel root commit");
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
static int get_author_identity(struct strbuf *result)
{
	struct child_process_def cmd;
	struct strbuf cmd_out;
	int ret = 0;

	strbuf_init(&cmd_out);
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;

	const char * const config_keys[] = {
			"user.username",
			"user.email",
			"user.name",
			NULL
	};

	const char * const *config_key = config_keys;
	while (*config_key) {
		strbuf_clear(&cmd_out);
		str_array_clear((struct str_array *)&cmd.args);

		argv_array_push(&cmd.args, "config", "--get", *config_key, NULL);
		ret = capture_command(&cmd, &cmd_out);
		if (!ret) {
			strbuf_trim(&cmd_out);
			strbuf_attach_str(result, cmd_out.buff);

			strbuf_release(&cmd_out);
			child_process_def_release(&cmd);

			return 0;
		}

		config_key++;
	}

	child_process_def_release(&cmd);
	strbuf_release(&cmd_out);

	return 1;
}
