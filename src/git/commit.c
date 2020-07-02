#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

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
static const char *parse_commit_header_oid(struct git_oid *oid, const char *data,
		size_t len, const char *header_prefix)
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
static int has_commit_header_oid(const char *data, size_t len, const char *header_prefix)
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
static const char *parse_commit_header_signature(struct git_signature *sig, const char *data,
		size_t len, const char *header_prefix)
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
			offset = strtoumax(tailptr + 1, &tailptr, 10);
			if (!tailptr || !isspace(*tailptr))
				return NULL;

			// if invalid, just assume zero
			if (offset == INTMAX_MAX || offset == INTMAX_MIN)
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
		sig->timestamp.offset = (offset / 100 * 60) + (offset % 100);
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
	return parse_commit_header_signature(NULL, data, len, header_prefix) != NULL;
}

int commit_parse(struct git_commit *commit, const char commit_id[GIT_HEX_OBJECT_ID],
		const char *data, size_t len)
{
	const char *current = data;
	size_t current_len = len;

	git_str_to_oid(&commit->commit_id, commit_id);

	// parse tree id (there should only ever be a single tree)
	current = parse_commit_header_oid(&commit->tree_id, current, current_len, "tree ");
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
		commit->parents_commit_ids = (struct git_oid *) realloc(commit->parents_commit_ids,
				commit->parents_commit_ids_len * sizeof(struct git_oid));
		if (!commit->parents_commit_ids)
			FATAL(MEM_ALLOC_FAILED);

		commit->parents_commit_ids[commit->parents_commit_ids_len - 1] = parent_id;
	}

	// parse author
	// might appear more than once, we'll just skip the extras
	current = parse_commit_header_signature(&commit->author, current, current_len, "author ");
	if (!current)
		return 1;

	current_len = data + len - current;

	while (has_commit_header_signature(current, current_len, "author ")) {
		current = parse_commit_header_signature(NULL, current, current_len, "author ");
		if (!current)
			break;

		current_len = data + len - current;
	}

	// parse committer
	current = parse_commit_header_signature(&commit->committer, current, current_len, "committer ");
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
