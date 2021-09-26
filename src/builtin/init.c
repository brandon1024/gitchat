#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "run-command.h"
#include "working-tree.h"
#include "git/git.h"
#include "git/index.h"
#include "git/commit.h"
#include "parse-options.h"
#include "config/parse-config.h"
#include "fs-utils.h"
#include "utils.h"
#include "version.h"

static const struct usage_string init_cmd_usage[] = {
		USAGE("git chat init [(-n | --name) <name>] [(-d | --description) <desc>]"),
		USAGE("git chat init (-h | --help)"),
		USAGE_END()
};

static int init(const char *, const char *);

int cmd_init(int argc, char *argv[])
{
	char *channel_name = NULL;
	char *room_desc = NULL;
	int show_help = 0;

	const struct command_option init_cmd_options[] = {
			OPT_STRING('n', "name", "name", "specify a name for the master channel", &channel_name),
			OPT_STRING('d', "description", "desc", "specify a description for the space", &room_desc),
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

	return init(channel_name, room_desc);
}

static void prepare_git_chat(const char *, const char *, const char *);
static void initialize_channel_root();

static int init(const char *channel_name, const char *space_desc)
{
	struct child_process_def cmd;
	struct strbuf author;

	if (is_inside_git_chat_space())
		DIE("a git-chat space cannot be reinitialized.");

	LOG_INFO("Initializing new space '%s' with master channel name '%s'",
			 space_desc, channel_name);

	// execute 'git init' as child process
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;

	argv_array_push(&cmd.args, "init", NULL);
	int ret = run_command(&cmd);
	if (ret)
		DIE("'git init' failed from the current directory; "
				"git exited with status %d", ret);

	child_process_def_release(&cmd);

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

	fprintf(stdout, "Successfully initialized git-chat space.\n");

	strbuf_release(&author);

	return 0;
}

static void update_config(char *, const char *, const char *);
static void update_space_description(char *, const char *);

/**
 * Try to find where the templates directory is installed. The following
 * directories are checked, in order of precedence:
 * - {GIT_CHAT_INSTALL_PREFIX}/share/git-chat/templates
 * - ${HOME}/share/git-chat/templates
 *
 * Returns zero if template directory was found, and non-zero otherwise.
 * */
static int find_templates_dir(struct strbuf *templates_dir)
{
	struct stat sb;

	const char *global_templates_dir = GIT_CHAT_INSTALL_PREFIX "/share/git-chat/templates";
	if (stat(global_templates_dir, &sb) != -1) {
		strbuf_attach_str(templates_dir, global_templates_dir);
		return 0;
	}

	const char *home = getenv("HOME");
	if (!home || !*home)
		DIE("Tried to copy from template directory $HOME/share/git-chat/templates, "
			"but the $HOME environment variable is not set.");

	strbuf_attach_fmt(templates_dir, "%s/share/git-chat/templates", home);
	if (stat(templates_dir->buff, &sb) == -1)
		return 1;

	return 0;
}

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
	struct strbuf templates_dir_path, path;

	strbuf_init(&templates_dir_path);
	strbuf_init(&path);

	if (find_templates_dir(&templates_dir_path)) {
		DIE("Something's not quite right with your installation.\n"
			"Could not find templates directory, typically located under %s.\n",
			GIT_CHAT_INSTALL_PREFIX "/share/git-chat/templates");
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
	struct config_data *conf;

	strbuf_init(&config_path);
	strbuf_attach_fmt(&config_path, "%s/config", base);

	config_data_init(&conf);
	int ret = parse_config(conf, config_path.buff);
	if (ret < 0)
		DIE("unable to update '%s'; cannot access file", config_path);
	if (ret > 0)
		DIE("unable to update '%s'; file contains syntax errors", config_path);

	if (channel_name) {
		int status = config_data_update(conf, "channel.\"master\".name", channel_name);
		if (status)
			DIE("unexpected config template with missing key 'channel.master.name'");
	}

	if (author) {
		int status = config_data_update(conf, "channel.\"master\".createdby", author);
		if (status)
			DIE("unexpected config template with missing key 'channel.master.createdby'");
	}

	if (write_config(conf, config_path.buff))
		DIE("failed to write config to file '%s'", config_path.buff);
	strbuf_release(&config_path);
	config_data_release(&conf);

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

	ssize_t len = strlen(description);
	if (xwrite(desc_fd, description, len) != len)
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
	struct strbuf git_chat_dir, config_path, description_path;;
	struct str_array files;

	strbuf_init(&git_chat_dir);
	if (get_git_chat_dir(&git_chat_dir))
		FATAL("unable to obtain .git-chat dir");

	strbuf_init(&config_path);
	strbuf_attach_fmt(&config_path, "%s/config", git_chat_dir.buff);

	strbuf_init(&description_path);
	strbuf_attach_fmt(&description_path, "%s/description", git_chat_dir.buff);

	str_array_init(&files);
	str_array_push(&files, config_path.buff, description_path.buff, NULL);

	int ret = git_add_files_to_index(&files);
	if (ret)
		DIE("failed to add files to the index; git exited with status %d", ret);

	strbuf_release(&git_chat_dir);
	strbuf_release(&config_path);
	strbuf_release(&description_path);
	str_array_release(&files);

	ret = git_commit_index_with_options("You have reached the beginning of time.",
			"--no-gpg-sign", "--no-verify", NULL);
	if (ret)
		DIE("failed to commit index to the tree; git exited with status %d", ret);

	LOG_DEBUG("Successfully created channel root commit");
}
