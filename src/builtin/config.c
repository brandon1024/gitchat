#include <stdio.h>

#include "parse-options.h"
#include "config/parse-config.h"
#include "config/config-defaults.h"
#include "config/config-key.h"
#include "run-command.h"
#include "working-tree.h"
#include "fs-utils.h"
#include "utils.h"

static struct usage_string usage[] = {
		USAGE("git chat config [--get] <key>"),
		USAGE("git chat config --get-or-default <key>"),
		USAGE("git chat config --set <key> <value>"),
		USAGE("git chat config --unset <key>"),
		USAGE("git chat config (--is-valid-key <key>) | --is-valid-config"),
		USAGE("git chat config (-e | --edit)"),
		USAGE("git chat config (-h | --help)"),
		USAGE_END()
};

static int config_query_value(const char *, int);
static int config_set_value(const char *, const char *);
static int editor_edit_config_file();
static int is_config_file_valid();

int cmd_config(int argc, char *argv[])
{
	int get_value = 0;
	int get_or_default = 0;
	int set_value = 0;
	int unset_value = 0;
	int edit = 0;
	int validate_key = 0;
	int is_valid_config = 0;
	int show_help = 0;

	const struct command_option config_cmd_options[] = {
			OPT_LONG_BOOL("get", "get config value with the given key", &get_value),
			OPT_LONG_BOOL("get-or-default", "get config value with the given key, or default value if not present", &get_or_default),
			OPT_LONG_BOOL("set", "create or mutate config value", &set_value),
			OPT_LONG_BOOL("unset", "delete config value with the given key", &unset_value),
			OPT_LONG_BOOL("is-valid-key", "exit with 0 if the key is valid", &validate_key),
			OPT_LONG_BOOL("is-valid-config", "exit with 0 if the config is valid", &is_valid_config),
			OPT_BOOL('e', "edit", "open an editor to edit the config file", &edit),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	argc = parse_options(argc, argv, config_cmd_options, 1, 1);
	if (show_help) {
		show_usage_with_options(usage, config_cmd_options, 0, NULL);
		return 0;
	}

	int opts = 0;
	if (get_value)
		opts++;
	if (get_or_default)
		opts++;
	if (set_value)
		opts++;
	if (unset_value)
		opts++;
	if (edit)
		opts++;
	if (validate_key)
		opts++;
	if (is_valid_config)
		opts++;

	if (opts > 1) {
		show_usage_with_options(usage, config_cmd_options, 1, "error: unexpected combination of options");
		return 1;
	}
	if (opts == 0)
		get_value = 1;

	if (get_value || get_or_default) {
		if (argc != 1) {
			show_usage_with_options(usage, config_cmd_options, 1, "error: wrong number of arguments, expected single config key");
			return 1;
		}

		return config_query_value(argv[0], get_or_default);
	}

	if (set_value) {
		if (argc != 2) {
			show_usage_with_options(usage, config_cmd_options, 1, "error: wrong number of arguments, expected config key and value");
			return 1;
		}

		return config_set_value(argv[0], argv[1]);
	}

	if (unset_value) {
		if (argc != 1) {
			show_usage_with_options(usage, config_cmd_options, 1, "error: wrong number of arguments, expected single config key");
			return 1;
		}

		return config_set_value(argv[0], NULL);
	}

	if (validate_key) {
		if (argc != 1) {
			show_usage_with_options(usage, config_cmd_options, 1, "error: wrong number of arguments, expected single config key");
			return 1;
		}

		return !(is_recognized_config_key(argv[0]) && is_valid_config_key(argv[0]));
	}

	if (is_valid_config) {
		if (argc != 0) {
			show_usage_with_options(usage, config_cmd_options, 1, "error: too many arguments");
			return 1;
		}

		return !is_config_file_valid();
	}

	if (edit) {
		if (argc) {
			show_usage_with_options(usage, config_cmd_options, 1, "error: too many options with --edit");
			return 1;
		}

		return editor_edit_config_file();
	}

	return 1;
}

static int config_query_value(const char *key, int fallback_default)
{
	struct strbuf cwd_path,  config_path;
	struct config_data *conf;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	strbuf_init(&cwd_path);
	if (get_cwd(&cwd_path))
		FATAL("unable to obtain the current working directory from getcwd()");

	strbuf_init(&config_path);
	strbuf_attach_fmt(&config_path, "%s/.git-chat/config", cwd_path.buff);

	config_data_init(&conf);
	int ret = parse_config(conf, config_path.buff);
	if (ret < 0)
		DIE("unable to query config from '%s'; cannot access file", config_path);
	if (ret > 0)
		DIE("unable to query config from '%s'; file contains syntax errors", config_path);

	const char *value = config_data_find(conf, key);
	if (value)
		fprintf(stdout, "%s\n", value);

	strbuf_release(&config_path);
	strbuf_release(&cwd_path);
	config_data_release(&conf);

	// if no entry was found, and fallback on default, then show default value
	if (!value && fallback_default) {
		const char *default_val = get_default_config_value(key);
		if (!default_val)
			return 1;

		fprintf(stdout, "%s\n", default_val);
		return 0;
	}

	return value == NULL;
}

static int config_set_value(const char *key, const char *value)
{
	struct strbuf cwd_path, config_path;
	struct config_data *conf;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	strbuf_init(&cwd_path);
	if (get_cwd(&cwd_path))
		FATAL("unable to obtain the current working directory from getcwd()");

	strbuf_init(&config_path);
	strbuf_attach_fmt(&config_path, "%s/.git-chat/config", cwd_path.buff);

	config_data_init(&conf);
	int ret = parse_config(conf, config_path.buff);
	if (ret < 0)
		FATAL("unable to update config '%s'; cannot access file", config_path);
	if (ret > 0)
		DIE("malformed config file '%s'", config_path);

	const char *old_value = config_data_find(conf, key);
	if (!value) {
		// unset config
		if (old_value) {
			int status = config_data_delete(conf, key);
			if (status < 0)
				DIE("invalid key '%s'", key);
			if (status > 0)
				DIE("no such config for key '%s'", key);
		} else {
			WARN("config with key '%s' does not exist", key);
		}
	} else if (!old_value) {
		// create if entry doesn't exist
		int status = config_data_insert(conf, key, value);
		if (status < 0)
			DIE("invalid key '%s'", key);
		if (status > 0)
			DIE("config already exists for key '%s'", key);
	} else {
		// set value if exists
		int status = config_data_update(conf, key, value);
		if (status < 0)
			DIE("invalid key '%s'", key);
		if (status > 0)
			DIE("config with key '%s' does not exist", key);
	}

	ret = write_config(conf, config_path.buff);
	if (ret < 0)
		FATAL("failed to open destination file '%s' for writing", config_path.buff);
	if (ret > 0)
		DIE("could not write malformed config", config_path.buff);

	strbuf_release(&config_path);
	strbuf_release(&cwd_path);
	config_data_release(&conf);

	// return 1 if --unset and entry not found
	return !value && !old_value;
}

static int editor_edit_config_file()
{
	struct strbuf cwd_path, config_path;
	struct child_process_def cmd;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	strbuf_init(&cwd_path);
	if (get_cwd(&cwd_path))
		FATAL("unable to obtain the current working directory from getcwd()");

	strbuf_init(&config_path);
	strbuf_attach_fmt(&config_path, "%s/.git-chat/config", cwd_path.buff);

	child_process_def_init(&cmd);
	cmd.executable = "vim";
	argv_array_push(&cmd.args, "-c", "set nobackup nowritebackup", "-n",
			config_path.buff, NULL);

	int status = run_command(&cmd);
	if (status)
		DIE("Vim editor failed with exit status '%d'", status);

	child_process_def_release(&cmd);

	strbuf_release(&config_path);
	strbuf_release(&cwd_path);

	return 0;
}

static int is_config_file_valid()
{
	struct strbuf cwd_path, config_path;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	strbuf_init(&cwd_path);
	if (get_cwd(&cwd_path))
		FATAL("unable to obtain the current working directory from getcwd()");

	strbuf_init(&config_path);
	strbuf_attach_fmt(&config_path, "%s/.git-chat/config", cwd_path.buff);

	int is_invalid = is_config_invalid(config_path.buff, 1);

	strbuf_release(&config_path);
	strbuf_release(&cwd_path);

	return !is_invalid;
}
