#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#include "parse-options.h"
#include "run-command.h"
#include "str-array.h"
#include "strbuf.h"
#include "config/parse-config.h"
#include "git/git.h"
#include "paging.h"
#include "utils.h"
#include "working-tree.h"

static const struct usage_string channel_cmd_usage[] = {
		USAGE("git chat channel list [(-a | --all)]"),
		USAGE_END()
};


struct channel_details {
	struct git_oid object_id;
	int message_count;
	char *refname_full;
	char *refname_short;
	char *origin;
	char *channel_name;
	char *channel_desc;
	unsigned remote: 1;
	unsigned current: 1;
};

/**
 * Initialize a channel details structure. Must be released with
 * `channel_details_release` after use.
 * */
static void channel_details_init(struct channel_details *channel,
		struct git_oid *oid, unsigned is_remote, unsigned is_current)
{
	channel->object_id = *oid;
	channel->remote = is_remote;
	channel->current = is_current;

	channel->message_count = -1;
	channel->channel_name = NULL;
	channel->channel_desc = NULL;
	channel->refname_full = NULL;
	channel->refname_short = NULL;
	channel->origin = NULL;
}

/**
 * Release a channel details structure.
 * */
static void channel_details_release(struct channel_details *channel)
{
	memset(channel->object_id.id, 0, GIT_RAW_OBJECT_ID);
	channel->remote = 0;
	channel->current = 0;
	channel->message_count = -1;

	free(channel->origin);
	free(channel->refname_short);
	free(channel->refname_full);
	free(channel->channel_desc);
	free(channel->channel_name);
}

/**
 * Calculate the message count for a given channel by git_oid and set
 * `*message_count` accordingly. `message_count` is only updated if the count
 * could be retrieved successfully.
 *
 * Returns zero of successful, and nonzero if an error occurred.
 * */
static int calculate_channel_message_count(struct git_oid *oid, int *message_count)
{
	struct child_process_def rev_list_cmd;
	child_process_def_init(&rev_list_cmd);
	rev_list_cmd.git_cmd = 1;

	char ref_id[GIT_HEX_OBJECT_ID + 1];
	git_oid_to_str(oid, ref_id);
	ref_id[GIT_HEX_OBJECT_ID] = 0;
	argv_array_push(&rev_list_cmd.args, "rev-list", "--count", "--first-parent",
			"--no-merges", ref_id, NULL);

	struct strbuf rev_list_out;
	strbuf_init(&rev_list_out);

	int status = capture_command(&rev_list_cmd, &rev_list_out);
	if (!status) {
		char *tailptr = NULL;
		unsigned long count = strtoul(rev_list_out.buff, &tailptr, 0);

		// verify that integer was parsed successfully
		if (tailptr && *tailptr == '\n') {
			*message_count = (int) count;
			LOG_TRACE("message count for channel with ref '%s': %d", ref_id, *message_count);
		} else {
			LOG_WARN("failed to parse channel message count for ref '%s'", ref_id);
		}
	}

	strbuf_release(&rev_list_out);
	child_process_def_release(&rev_list_cmd);
	return status;
}

/**
 * Parse the git-chat config file for a given channel and update the appropriate
 * fields in `channel`.
 *
 * Returns zero if successful, and nonzero if unable to read or parse the
 * config file for that channel.
 * */
static int parse_channel_config(struct git_oid *oid, const char *branch_name,
		char **name, char **desc)
{
	if (!branch_name)
		return 1;

	struct child_process_def show_config_cmd;
	child_process_def_init(&show_config_cmd);
	show_config_cmd.git_cmd = 1;

	char ref_id[GIT_HEX_OBJECT_ID + 1];
	git_oid_to_str(oid, ref_id);
	ref_id[GIT_HEX_OBJECT_ID] = 0;

	/*
	 * lookup config file for the given channel and write to temporary file.
	 * */
	struct strbuf config_file;
	strbuf_init(&config_file);
	strbuf_attach_fmt(&config_file, "%s:%s", ref_id, ".git-chat/config");

	argv_array_push(&show_config_cmd.args, "--no-pager", "show", config_file.buff, NULL);
	strbuf_release(&config_file);

	char tmp_file_name_template[] = "/tmp/gitchat_XXXXXX";
	int tmp_fd = mkstemp(tmp_file_name_template);
	if (tmp_fd < 0)
		FATAL("unable to create temporary file");
	if (unlink(tmp_file_name_template) < 0)
		FATAL("failed to unlink temporary file from filesystem");

	child_process_def_stderr(&show_config_cmd, STDERR_NULL);
	child_process_def_stdout(&show_config_cmd, STDOUT_PROVISIONED);
	show_config_cmd.out_fd[0] = -1;
	show_config_cmd.out_fd[1] = tmp_fd;

	start_command(&show_config_cmd);
	int status = finish_command(&show_config_cmd);
	if (status) {
		LOG_ERROR("unable to read config file for channel with oid '%.*s'; "
				  "child process exited with status %d", GIT_HEX_OBJECT_ID,
				ref_id, status);

		child_process_def_release(&show_config_cmd);
		close(tmp_fd);
		return 1;
	}

	child_process_def_release(&show_config_cmd);

	off_t offset = lseek(tmp_fd, 0, SEEK_SET);
	if (offset < 0)
		FATAL("failed to seek temp file descriptor");

	struct config_data *config;
	config_data_init(&config);
	status = parse_config_fd(config, tmp_fd);
	if (status) {
		LOG_ERROR("unable to parse config file for channel with oid '%.*s'",
				GIT_HEX_OBJECT_ID, ref_id);

		close(tmp_fd);
		return 1;
	}

	const char *prop_value = config_data_find_exp_key(config, "channel", branch_name, "name", NULL);
	if (prop_value) {
		*name = strdup(prop_value);
		if (!*name)
			FATAL(MEM_ALLOC_FAILED);
	} else {
		LOG_WARN("could not find channel name from config file for '%.*s'",
				GIT_HEX_OBJECT_ID, ref_id);
	}

	prop_value = config_data_find_exp_key(config, "channel", branch_name, "description", NULL);
	if (prop_value) {
		*desc = strdup(prop_value);
		if (!*desc)
			FATAL(MEM_ALLOC_FAILED);
	} else {
		LOG_WARN("could not find channel description from config file for '%.*s'",
				GIT_HEX_OBJECT_ID, ref_id);
	}

	config_data_release(&config);

	close(tmp_fd);

	return 0;
}

/**
 * Parse a git reference into its parts (origin, name).
 *
 * The `refname_full` is expected to have the form `refs/heads/<name>` or
 * `refs/remotes/<origin>/<name>`. When `remote` is non-zero, the latter refname
 * is expected.
 *
 * Returns zero if successfully parsed components, and nonzero otherwise.
 * */
static int parse_ref(const char *refname_full, char **origin, char **name, unsigned remote)
{
	int status = 0;

	struct strbuf ref_parse_buf;
	strbuf_init(&ref_parse_buf);
	strbuf_attach_str(&ref_parse_buf, refname_full);

	struct str_array ref_components;
	str_array_init(&ref_components);

	strbuf_split(&ref_parse_buf, "/", &ref_components);
	if (remote) {
		// expect four or more components (refs/remotes/<remote>/<name>)
		if (ref_components.len < 4) {
			LOG_WARN("unexpected remote ref '%s'; expected at least 4 distinct components", refname_full);
			status = 1;
			goto end;
		}

		*origin = str_array_get(&ref_components, 2);
		*origin = strdup(*origin);
		if (!*origin)
			FATAL(MEM_ALLOC_FAILED);

		str_array_delete(&ref_components, 0, 3);
		*name = argv_array_collapse_delim((struct argv_array *)&ref_components, "/");
	} else {
		// expect three or more parts (refs/heads/<name>)
		if (ref_components.len < 3) {
			LOG_WARN("unexpected local ref '%s'; expected at least 3 distinct components", refname_full);
			status = 1;
			goto end;
		}

		str_array_delete(&ref_components, 0, 2);
		*name = argv_array_collapse_delim((struct argv_array *)&ref_components, "/");
	}

end:
	str_array_release(&ref_components);
	strbuf_release(&ref_parse_buf);

	return status;
}

/**
 * Fetch channel details and allocate `details`. `oid` must represent the tip
 * of a given channel. `refname` must be the full refname for the channel
 * (refs/heads/<channel> if remote = 0, refs/remotes/<remote>/<channel> if
 * remote = 1).
 *
 * This function will try to populate as much information about the channel as
 * possible. If unable to fetch information, this function will still succeed,
 * with that data simply omitted. It's up to the caller to handle NULLs.
 * */
static void fetch_channel_details(struct git_oid *oid, const char *refname,
		struct channel_details **details, unsigned current, unsigned remote)
{
	struct channel_details *channel = (struct channel_details *)malloc(sizeof(struct channel_details));
	if (!channel)
		FATAL(MEM_ALLOC_FAILED);

	channel_details_init(channel, oid, remote, current);

	channel->refname_full = strdup(refname);
	if (!channel->refname_full)
		FATAL(MEM_ALLOC_FAILED);

	if (parse_ref(refname, &channel->origin, &channel->refname_short, remote))
		LOG_WARN("failed to parse ref '%s'", refname);
	if (calculate_channel_message_count(oid, &channel->message_count))
		LOG_WARN("failed to retrieve message count for channel with ref '%s'", refname);
	if (parse_channel_config(oid, channel->refname_short, &channel->channel_name, &channel->channel_desc))
		LOG_WARN("something went wrong when parsing config file for channel with ref '%s'", refname);

	*details = channel;
}

/**
 * Construct a list of channel refs matching the given refname pattern.
 *
 * The `channel_refs` argument is populated with the refname for each channel.
 * The `channel_refs` data fields are populated with more granular information
 * about each channel, like the channel name, description, number of messages, etc.
 *
 * `refname_pattern` represents a ref pattern acceptable by git-for-each-ref.
 * When `remote` is zero, the refname pattern "refs/heads" is expected. When
 * `remote` is non-zero, the refname pattern "refs/remotes" is expected.
 *
 * Returns zero if successful, and non-zero if:
 * - git for-each-ref failed with non-zero exit status, or
 * - fetch_channel_details() was unable to assemble channel details
 * */
static int fetch_channels(struct str_array *channel_refs,
		const char *refname_pattern, unsigned remote)
{
	struct child_process_def show_ref_cmd;
	child_process_def_init(&show_ref_cmd);
	show_ref_cmd.git_cmd = 1;

	argv_array_push(&show_ref_cmd.args, "for-each-ref", "--format=%(objectname) %(HEAD) %(refname)",
			refname_pattern, NULL);

	struct strbuf show_ref_output;
	strbuf_init(&show_ref_output);

	struct str_array ref_out_lines;
	str_array_init(&ref_out_lines);

	int status = capture_command(&show_ref_cmd, &show_ref_output);
	child_process_def_release(&show_ref_cmd);

	if (status)
		goto fail;

	size_t line_count = strbuf_split(&show_ref_output, "\n", &ref_out_lines);
	for (size_t i = 0; i < line_count; i++) {
		char *line = str_array_get(&ref_out_lines, i);
		size_t line_len = strlen(line);

		// stop processing if no refs left
		if (!line_len)
			break;

		LOG_DEBUG("processing %s ref '%s'", remote ? "remote" : "local", line);

		// panic if line does not have format '<sha 1> <*> <refname>'
		if (line_len < (GIT_HEX_OBJECT_ID + 3))
			FATAL("unexpected output from git for-each-ref: '%s'", line);

		struct git_oid ref_oid;
		git_str_to_oid(&ref_oid, line);

		unsigned is_current = line[GIT_HEX_OBJECT_ID + 1] == '*';
		char *refname = line + GIT_HEX_OBJECT_ID + 3;
		struct str_array_entry *entry = str_array_insert(channel_refs, refname, 0);

		// fetch information about channel
		struct channel_details *details;
		fetch_channel_details(&ref_oid, refname, &details, is_current, remote);
		entry->data = details;
	}

fail:
	strbuf_release(&show_ref_output);
	str_array_release(&ref_out_lines);

	return status;
}

/**
 * Fetch all channels with the refname pattern `refs/heads`. Refer to
 * `fetch_channels` implementation for further details.
 * */
static int fetch_local_channels(struct str_array *channel_refs)
{
	return fetch_channels(channel_refs, "refs/heads", 0);
}

/**
 * Fetch all channels with the refname pattern `refs/remotes`. Refer to
 * `fetch_channels` implementation for further details.
 * */
static int fetch_remote_channels(struct str_array *channel_refs)
{
	return fetch_channels(channel_refs, "refs/remotes", 1);
}

/**
 * Filter any remote channels that aren't particularly interesting. That is,
 * remote channels that shadow local ones, where both refs are identical
 * (reference same commit and have the same short refname).
 * */
static void filter_uninteresting_remote_channels(struct str_array *channels)
{
	for (size_t i = 0; i < channels->len;) {
		struct str_array_entry *entry = str_array_get_entry(channels, i);
		struct channel_details *detail = entry->data;

		if (!detail->remote) {
			i++;
			continue;
		}

		// try to find a local channel with matching (short) name and oid
		int remote_shadows_local = 0;
		for (size_t j = 0; j < channels->len; j++) {
			struct str_array_entry *local_entry = str_array_get_entry(channels, j);
			struct channel_details *local_detail = local_entry->data;

			if (local_detail->remote)
				continue;

			if (memcmp(local_detail->object_id.id, detail->object_id.id, GIT_RAW_OBJECT_ID) != 0)
				continue;
			if (strcmp(local_detail->refname_short, detail->refname_short) != 0)
				continue;

			remote_shadows_local = 1;
			break;
		}

		if (remote_shadows_local) {
			LOG_DEBUG("remote ref '%s' shadows local copy, so skipping it from the channel listing",
					detail->refname_full);

			str_array_delete(channels, i, 1);

			channel_details_release(detail);
			free(detail);
		} else {
			i++;
		}
	}
}

struct table_dimensions {
	int refname;
	int message_count;
	int channel_name;
};

/**
 * Calculate the width (in characters) for each column. Needed when displaying
 * channels in a tablulated format.
 * */
static void calculate_table_dimensions(struct str_array *channels,
		struct table_dimensions *dims)
{
	dims->refname = 0;
	dims->message_count = 0;
	dims->channel_name = 0;

	for (size_t i = 0; i < channels->len; i++) {
		struct str_array_entry *entry = str_array_get_entry(channels, i);
		struct channel_details *detail = entry->data;

		if (detail->refname_short) {
			size_t refname_len = strlen(detail->refname_short);
			if (refname_len > dims->refname)
				dims->refname = refname_len;
		}

		if (detail->message_count > 0) {
			int count_len = (int) floor(log10(detail->message_count));
			if (count_len > dims->message_count)
				dims->message_count = count_len;
		} else if (!detail->message_count) {
			if (dims->message_count < 1)
				dims->message_count = 1;
		}

		if (detail->channel_name) {
			size_t channel_name_len = strlen(detail->channel_name);
			if (channel_name_len > dims->channel_name)
				dims->channel_name = channel_name_len;
		}
	}
}

/**
 * Print local or remote channels, formatted in a table.
 *
 * Channels are removed from the `channels` array when printed. If remote is
 * non-zero, only channels with the matching `origin` are printed.
 * */
static void print_channels(struct str_array *channels, struct table_dimensions *dims,
		unsigned remote, char *origin)
{
	size_t i = 0;
	while (i < channels->len) {
		struct str_array_entry *entry = str_array_get_entry(channels, i);
		struct channel_details *detail = entry->data;

		if (remote ^ detail->remote) {
			i++;
			continue;
		}

		if (remote && strcmp(origin, detail->origin) != 0) {
			i++;
			continue;
		}

		// print '*' if this channel is currently checked out
		printf("%s", detail->current ? "* " : "  ");

		// print branch name
		if (detail->refname_short) {
			if (detail->current)
				printf(ANSI_COLOR_GREEN);
			if (detail->remote)
				printf(ANSI_COLOR_RED);

			size_t refname_short_len = strlen(detail->refname_short);
			printf("%s", detail->refname_short);
			printf("%*s", dims->refname - (int)refname_short_len, "");

			if (detail->current || detail->remote)
				printf(ANSI_COLOR_RESET);
		} else {
			printf("%*s", dims->refname, "");
		}

		// print message count
		if (detail->message_count >= 0)
			printf(" [%*d] ", dims->message_count, detail->message_count);
		else
			printf(" [%*s]", dims->message_count, "");

		// print channel name, truncated to 20 characters
		if (detail->channel_name) {
			size_t channel_name_len = strlen(detail->channel_name);
			printf("[%s", detail->channel_name);
			printf("%*s] ", dims->channel_name - (int)channel_name_len, "");
		} else {
			printf("[%*s] ", dims->channel_name, "");
		}

		// print description, truncated to 40 characters
		if (detail->channel_desc) {
			size_t channel_name_len = strlen(detail->channel_desc);
			if (channel_name_len <= 40)
				printf("%s", detail->channel_desc);
			else
				printf("%.*s...", 37, detail->channel_desc);
		} else {
			printf("%*s", 40, "");
		}

		channel_details_release(detail);
		free(detail);
		str_array_delete(channels, i, 1);

		printf("\n");
	}
}

int channel_list(int argc, char *argv[])
{
	int show_help = 0;
	int show_all = 0;

	const struct command_option channel_cmd_options[] = {
			OPT_BOOL('a', "all", "list all channels, even if in sync with remote", &show_all),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	argc = parse_options(argc, argv, channel_cmd_options, 0, 1);
	if (show_help) {
		show_usage_with_options(channel_cmd_usage, channel_cmd_options, 0, NULL);
		return 0;
	}

	if (argc) {
		show_usage_with_options(channel_cmd_usage, channel_cmd_options, 1, "error: unknown option '%s'", argv[0]);
		return 1;
	}

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	struct str_array channel_refs;
	str_array_init(&channel_refs);

	int status = fetch_local_channels(&channel_refs);
	if (status)
		FATAL("something went wrong when fetching local channels");
	status = fetch_remote_channels(&channel_refs);
	if (status)
		FATAL("something went wrong when fetching remote channels");

	// filter duplicate remote channels (remote channels that are up to date with local)
	if (!show_all)
		filter_uninteresting_remote_channels(&channel_refs);

	// sort
	str_array_sort(&channel_refs);

	struct table_dimensions dims;
	calculate_table_dimensions(&channel_refs, &dims);

	pager_start(GIT_CHAT_PAGER_DEFAULT);

	// print local channels
	printf("local channels\n");
	print_channels(&channel_refs, &dims, 0, NULL);

	// print channels for each remote
	struct strbuf origin;
	strbuf_init(&origin);
	while (channel_refs.len > 0) {
		struct str_array_entry *entry = str_array_get_entry(&channel_refs, 0);
		struct channel_details *detail = entry->data;

		if (!detail->remote)
			BUG("all local channels should have been filtered");

		strbuf_attach_str(&origin, detail->origin);
		printf("\nremote channels [%s]\n", detail->origin);
		print_channels(&channel_refs, &dims, 1, origin.buff);

		strbuf_clear(&origin);
	}

	strbuf_release(&origin);
	str_array_release(&channel_refs);

	return status;
}
