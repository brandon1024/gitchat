#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "git/commit.h"
#include "run-command.h"
#include "utils.h"

void git_commit_object_init(struct git_commit *commit)
{
	memset(commit->commit_id.id, 0, GIT_RAW_OBJECT_ID);
	memset(commit->tree_id.id, 0, GIT_RAW_OBJECT_ID);

	commit->parents_commit_ids_len = 0;
	commit->parents_commit_ids = NULL;

	strbuf_init(&commit->committer.name);
	strbuf_init(&commit->committer.email);
	commit->committer.timestamp.time = 0;
	commit->committer.timestamp.offset = 0;

	strbuf_init(&commit->author.name);
	strbuf_init(&commit->author.email);
	commit->author.timestamp.time = 0;
	commit->author.timestamp.offset = 0;

	strbuf_init(&commit->body);
}

void git_commit_object_release(struct git_commit *commit)
{
	if (commit->parents_commit_ids)
		free(commit->parents_commit_ids);

	memset(commit->commit_id.id, 0, GIT_RAW_OBJECT_ID);
	memset(commit->tree_id.id, 0, GIT_RAW_OBJECT_ID);

	commit->parents_commit_ids_len = 0;
	commit->parents_commit_ids = NULL;

	strbuf_release(&commit->committer.name);
	strbuf_release(&commit->committer.email);
	commit->committer.timestamp.time = 0;
	commit->committer.timestamp.offset = 0;

	strbuf_release(&commit->author.name);
	strbuf_release(&commit->author.email);
	commit->author.timestamp.time = 0;
	commit->author.timestamp.offset = 0;

	strbuf_release(&commit->body);
}

int git_commit_index(const char *commit_message)
{
	return git_commit_index_with_options(commit_message, NULL);
}

int git_commit_index_with_options(const char *commit_message, ...)
{
	va_list varargs;
	struct child_process_def cmd;
	const char *arg;
	int ret;

	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "commit", "-m", commit_message, NULL);

	va_start(varargs, commit_message);
	while ((arg = va_arg(varargs, char *)))
		argv_array_push(&cmd.args, arg, NULL);

	va_end(varargs);

	ret = run_command(&cmd);
	child_process_def_release(&cmd);

	return ret;
}

/**
 * Attempt to parse a git object id from the given data buffer. The data is
 * assumed to be prefixed with the given header prefix.
 *
 * Returns NULL if the git oid couldn't be parsed. Otherwise, returns a pointer
 * to the next character in the buffer after the line feed at the end of the
 * header line.
 * */
static const char *parse_commit_header_oid(struct git_oid *oid,
		const char *data, size_t len, const char *header_prefix)
{
	const char *current = data;
	size_t current_len = len;

	size_t header_prefix_len = strlen(header_prefix);
	if (len < header_prefix_len)
		return NULL;

	if (memcmp(data, header_prefix, header_prefix_len) != 0)
		return NULL;

	current += header_prefix_len;
	current_len -= header_prefix_len;
	if (current_len < GIT_HEX_OBJECT_ID)
		return NULL;

	if (oid)
		git_str_to_oid(oid, current);
	return current + GIT_HEX_OBJECT_ID + 1;
}

/**
 * Check whether a git object id can be parsed from the given buffer. The data is
 * assumed to be prefixed with the given header prefix.
 *
 * Returns non-zero if an object ID can be parsed, and zero if not.
 * */
static int has_commit_header_oid(const char *data, size_t len,
		const char *header_prefix)
{
	return parse_commit_header_oid(NULL, data, len, header_prefix) != NULL;
}

/**
 * Parse a signature from a commit object header into the strbuf.
 *
 * Returns NULL if the signature couldn't be parsed. Otherwise, returns a pointer
 * to the next character in the buffer after the line feed at the end of the
 * header line.
 * */
static const char *parse_commit_header_signature(struct git_signature *sig,
		const char *data, size_t len, const char *header_prefix)
{
	const char *current = data;
	size_t current_len = len;

	size_t header_prefix_len = strlen(header_prefix);
	if (len < header_prefix_len)
		return NULL;

	if (memcmp(data, header_prefix, header_prefix_len) != 0)
		return NULL;

	current += header_prefix_len;
	current_len -= header_prefix_len;
	const char *lf = memchr(current, '\n', current_len);
	if (!lf)
		return NULL;

	const char *email_start = memchr(current, '<', lf - current);
	if (!email_start)
		return NULL;
	const char *email_end = memchr(email_start, '>', lf - email_start);
	if (!email_end)
		return NULL;

	intmax_t unix_epoch = 0;
	intmax_t offset = 0;
	if (email_end + 2 < lf) {
		const char *timestamp_start = email_end + 2;
		char *tailptr = NULL;
		unix_epoch = strtoimax(timestamp_start, &tailptr, 10);
		if (!tailptr || !isspace(*tailptr))
			return NULL;

		// if invalid, just assume zero
		if (unix_epoch == INTMAX_MAX || unix_epoch == INTMAX_MIN)
			unix_epoch = 0;

		if (tailptr + 1 < lf) {
			offset = strtoimax(tailptr + 1, &tailptr, 10);
			if (!tailptr || !isspace(*tailptr))
				return NULL;

			// if invalid, just assume zero
			if (offset < -2400 || offset > 2400)
				offset = 0;
		}
	}

	if (sig) {
		strbuf_attach(&sig->name, current, email_start - current);
		strbuf_trim(&sig->name);

		if ((email_start + 1) < email_end) {
			size_t email_len = email_end - (email_start + 1);
			strbuf_attach(&sig->email, email_start + 1, email_len);
			strbuf_trim(&sig->email);
		}

		sig->timestamp.time = unix_epoch;

		// offsets are in the format (+|-)hhmm, so we need to massage
		// the value a bit
		sig->timestamp.offset =
				((int) offset / 100 * 60) + ((int) offset % 100);
	}

	return lf + 1;
}

/**
 * Check whether a signature can be parsed from the given buffer. The data is
 * assumed to be prefixed with the given header prefix.
 *
 * Returns non-zero if a signature can be parsed, and zero if not.
 * */
static int has_commit_header_signature(const char *data, size_t len,
		const char *header_prefix)
{
	return parse_commit_header_signature(NULL, data, len, header_prefix)
			!= NULL;
}

int commit_parse(struct git_commit *commit,
		const char commit_id[GIT_HEX_OBJECT_ID], const char *data, size_t len)
{
	const char *current = data;
	size_t current_len = len;

	git_str_to_oid(&commit->commit_id, commit_id);

	// parse tree id (there should only ever be a single tree)
	current = parse_commit_header_oid(&commit->tree_id, current, current_len,
			"tree ");
	if (!current)
		return 1;

	current_len = data + len - current;

	// parse parent commit ids
	// one or more parents on separate lines
	while (has_commit_header_oid(current, current_len, "parent ")) {
		struct git_oid parent_id;
		current = parse_commit_header_oid(&parent_id, current, len, "parent ");
		if (!current)
			return 1;

		current_len = data + len - current;

		// 90% of the time, only one parent commit exists, so growing the
		// array by one is sufficient
		commit->parents_commit_ids_len += 1;
		commit->parents_commit_ids = (struct git_oid *) realloc(
				commit->parents_commit_ids,
				commit->parents_commit_ids_len * sizeof(struct git_oid));
		if (!commit->parents_commit_ids)
			FATAL(MEM_ALLOC_FAILED);

		commit->parents_commit_ids[commit->parents_commit_ids_len
				- 1] = parent_id;
	}

	// parse author
	// might appear more than once, we'll just skip the extras
	current = parse_commit_header_signature(&commit->author, current,
			current_len, "author ");
	if (!current)
		return 1;

	current_len = data + len - current;

	while (has_commit_header_signature(current, current_len, "author ")) {
		current = parse_commit_header_signature(NULL, current, current_len,
				"author ");
		if (!current)
			break;

		current_len = data + len - current;
	}

	// parse committer
	current = parse_commit_header_signature(&commit->committer, current,
			current_len, "committer ");
	if (!current)
		return 1;

	current_len = data + len - current;

	// skim additional header entries
	while (current_len > 0) {
		const char *lf = current;
		if (current[-1] == '\n' && current[0] == '\n')
			break;

		while (lf < (data + len) && *lf != '\n')
			++lf;
		if (lf < (data + len) && *lf == '\n')
			++lf;
		current = lf;
		current_len = data + len - current;
	}

	// finally, include commit message body
	strbuf_attach(&commit->body, current, current_len);
	strbuf_trim(&commit->body);
	return 0;
}

/**
 * Format the header for a message, and copy the result to `header_buff`.
 * */
static void format_pretty_message_header(struct strbuf *header_buff,
		struct git_commit *commit, enum message_type type, int no_color)
{
	const char *color_reset = no_color ? "" : ANSI_COLOR_RESET;
	const char *color_enable = no_color ? "" : ANSI_COLOR_RED;
	const char *flag_str = "???";

	// if colored output is enabled, determine the color based on the message type
	switch (type) {
		case DECRYPTED:
			flag_str = "DEC";
			color_enable = ANSI_COLOR_GREEN;
			break;
		case PLAINTEXT:
			flag_str = "PLN";
			color_enable = ANSI_COLOR_CYAN;
			break;
		case UNKNOWN_ERROR:
			flag_str = "ERR";
			color_enable = ANSI_COLOR_RED;
			break;
		default:
			flag_str = "???";
			color_enable = ANSI_COLOR_RED;
	}

	color_enable = no_color ? "" : color_enable;

	struct strbuf meta;
	strbuf_init(&meta);

	time_t epoch = commit->author.timestamp.time;
	struct tm *info;
	info = localtime(&epoch);

	strbuf_attach_fmt(&meta, "%s", asctime(info));
	strbuf_trim(&meta);
	strbuf_attach_fmt(&meta, " %s %s", flag_str, commit->author.name.buff);
	strbuf_trim(&meta);

	strbuf_attach_fmt(header_buff, "%s[%s]%s\n", color_enable, meta.buff,
			color_reset);

	strbuf_release(&meta);
}

/**
 * Format `message` and write the formatted message to `body_buff`.
 * */
static void format_pretty_message_body(struct strbuf *body_buff,
		struct strbuf *message)
{
	struct strbuf tmp_message_buff;
	struct str_array lines;

	strbuf_init(&tmp_message_buff);
	str_array_init(&lines);

	// copy `message` to temporary buffer and trim whitespace
	strbuf_attach(&tmp_message_buff, message->buff, message->len);
	strbuf_trim(&tmp_message_buff);

	// split message into lines
	int line_count = strbuf_split(&tmp_message_buff, "\n", &lines);
	strbuf_clear(&tmp_message_buff);

	// indent lines
	for (int i = 0; i < line_count; i++) {
		char *line = str_array_get(&lines, i);
		strbuf_attach_fmt(&tmp_message_buff, "\n\t%s", line);
	}

	str_array_release(&lines);

	// finally attach to result buffer
	strbuf_attach_fmt(body_buff, "%s\n\n", tmp_message_buff.buff);

	strbuf_release(&tmp_message_buff);
}

void pretty_print_message(struct git_commit *commit, struct strbuf *message,
		enum message_type type, int no_color, int output_fd)
{
	struct strbuf header, body;
	strbuf_init(&header);
	strbuf_init(&body);

	// format header
	format_pretty_message_header(&header, commit, type, no_color);

	// format body
	format_pretty_message_body(&body, message);

	// write to file descriptor
	if (xwrite(output_fd, header.buff, header.len) != header.len)
		FATAL("failed to write to file descriptor");
	if (xwrite(output_fd, body.buff, body.len) != body.len)
		FATAL("failed to write to file descriptor");

	strbuf_release(&header);
	strbuf_release(&body);
}
