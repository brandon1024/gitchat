#include "config/parse-config.h"
#include "git/commit.h"
#include "git/index.h"
#include "parse-options.h"
#include "argv-array.h"
#include "run-command.h"
#include "fs-utils.h"
#include "utils.h"
#include "working-tree.h"

static const struct usage_string channel_cmd_usage[] = {
		USAGE("git chat channel create [(-n | --name) <alias>] [(-d | --description) <description>] <refname>"),
		USAGE_END()
};

int channel_create(int argc, char *argv[])
{
	int show_help = 0;
	char *alias = NULL;
	char *description = NULL;

	const struct command_option channel_cmd_options[] = {
			OPT_STRING('n', "name", "alias", "specify channel name", &alias),
			OPT_STRING('d', "description", "description",
					"specify channel description", &description),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	argc = parse_options(argc, argv, channel_cmd_options, 0, 1);
	if (show_help) {
		show_usage_with_options(channel_cmd_usage, channel_cmd_options, 0,
				NULL);
		return 0;
	}

	if (argc != 1) {
		const char *err_msg = !argc ? "error: too few options"
				: "error: too many options";
		show_usage_with_options(channel_cmd_usage, channel_cmd_options, 1,
				err_msg);
		return 1;
	}

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	if (!alias)
		alias = argv[0];

	const char *channel_name = argv[0];

	struct config_data *conf;
	config_data_init(&conf);

	struct strbuf config_path;
	strbuf_init(&config_path);
	if (get_cwd(&config_path))
		FATAL("unable to obtain the current working directory from getcwd()");

	strbuf_attach_str(&config_path, "/.git-chat/config");

	int status = parse_config(conf, config_path.buff);
	if (status)
		DIE("couldn't parse the git-chat config file; check the config file syntax.");

	struct strbuf author;
	strbuf_init(&author);
	if (get_author_identity(&author)) {
		WARN("Unable to retrieve your user git information.\n"
			 "Is your user configured through .gitconfig?");
	}

	const char *err_msg = "something went wrong when trying to update the git-chat config;\n"
						  "it's possible the channel already exists with the same name, or\n"
						  "the channel contains characters not supported by git-chat";
	if (config_data_insert_exp_key(conf, author.buff, "channel", channel_name,
			"createdby", NULL))
		DIE(err_msg);
	if (config_data_insert_exp_key(conf, alias, "channel", channel_name, "name",
			NULL))
		DIE(err_msg);
	if (description && config_data_insert_exp_key(conf, description, "channel", channel_name,
			"description", NULL))
		DIE(err_msg);

	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "checkout", "-b", channel_name, NULL);

	status = run_command(&cmd);
	child_process_def_release(&cmd);

	if (status)
		DIE("failed to create a new channel; does this channel exist?");

	status = write_config(conf, config_path.buff);
	if (status)
		DIE("failed to write the git-chat config file '%s'", config_path.buff);

	status = git_add_file_to_index(config_path.buff);
	if (status)
		DIE("failed to update index with config file '%s'", config_path.buff);

	struct strbuf commit_message;
	strbuf_init(&commit_message);

	strbuf_attach_fmt(&commit_message,
			"You have reached the beginning of channel '%s'.", alias);

	status = git_commit_index_with_options(commit_message.buff, "--no-gpg-sign",
			"--no-verify", NULL);
	if (status)
		DIE("failed to commit index to the tree; git exited with status %d",
				status);

	LOG_INFO("successfully created new channel ref '%s'", channel_name);

	strbuf_release(&author);
	strbuf_release(&commit_message);
	strbuf_release(&config_path);
	config_data_release(&conf);

	return status;
}
