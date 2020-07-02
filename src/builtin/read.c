#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#include "parse-options.h"
#include "run-command.h"
#include "str-array.h"
#include "strbuf.h"
#include "paging.h"
#include "utils.h"
#include "working-tree.h"
#include "gnupg/gpg-common.h"
#include "git/commit.h"

#define READ 0
#define WRITE 1
#define BUFF_LEN 1024
#define NONCE_LEN 16

static const struct usage_string read_cmd_usage[] = {
		USAGE("git chat read [<commit hash>]"),
		USAGE("git chat read (-h | --help)"),
		USAGE_END()
};

static int read_messages(const char *);
static void str_template_generate_nonce(char[], char[], size_t);
static int parse_git_cat_file_output(struct str_array *, struct strbuf *,
		char[NONCE_LEN]);

int cmd_read(int argc, char *argv[])
{
	int show_help = 0;

	const struct command_option options[] = {
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	argc = parse_options(argc, argv, options, 1, 1);
	if (show_help) {
		show_usage_with_options(read_cmd_usage, options, 0, NULL);
		return 0;
	}

	if (argc > 1) {
		show_usage_with_options(read_cmd_usage, options, 1,
				"error: only one message may be read at a time.");
		return 1;
	}

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	if (argc)
		return read_messages(argv[0]);
	else
		return read_messages(NULL);
}

/**
 * Pretty-print a single message.
 *
 * Messages are displayed in the following format:
 * [<timestamp> <ENC | PLN> <author>]
 *   <message>...
 *   <message>...
 * */
static void pretty_print_message(struct git_commit *commit, struct strbuf *message,
		int flag)
{
	struct strbuf meta, formatted_message;
	strbuf_init(&meta);
	strbuf_init(&formatted_message);

	time_t epoch = commit->author.timestamp.time;

	const char *flag_str;
	switch (flag) {
		case 0:
			flag_str = "DEC";
			break;
		case 1:
			flag_str = "PLN";
			break;
		case 2:
			flag_str = "ERR";
			break;
		default:
			flag_str = "???";
	}

	struct tm *info;
	info = localtime(&epoch);
	strbuf_attach_fmt(&meta, "%s", asctime(info));
	strbuf_trim(&meta);
	strbuf_attach_fmt(&meta, " %s %s", flag_str, commit->author.name.buff);

	strbuf_trim(message);

	struct str_array lines;
	str_array_init(&lines);

	int line_count = strbuf_split(message, "\n", &lines);
	for (int i = 0; i < line_count; i++) {
		char *line = str_array_get(&lines, i);
		strbuf_attach_fmt(&formatted_message, "\n\t%s", line);
	}

	printf("[%s]%s\n", meta.buff, formatted_message.buff);

	strbuf_release(&meta);
	strbuf_release(&formatted_message);
}

/**
 * Decrypt and display messages in reverse-chronological order. If the commit_id
 * argument is NULL, displays all messages, otherwise simply displays the specific
 * message.
 *
 * Returns zero if successful, and non-zero otherwise.
 * */
static int read_messages(const char *commit_id)
{
	struct child_process_def rev_list;
	struct child_process_def cat_file;
	struct strbuf cat_file_out_buf;
	struct str_array parsed_commits;

	struct gc_gpgme_ctx ctx;

	gpgme_context_init(&ctx, 0);

	ssize_t bytes_read;
	int rev_list_exit, cat_file_exit;

	strbuf_init(&cat_file_out_buf);
	str_array_init(&parsed_commits);

	/* git-rev-list to read commit objects in reverse chronological order.
	 *
	 * Traverse the commit graph following the first parent if merge commits are
	 * encountered, and skipping such merge commits. We are relying on having a
	 * pretty clean commit graph here, and this might start to break down if the
	 * user tries to mess with the commit graph (introducing merges, for instance).
	 */
	child_process_def_init(&rev_list);
	rev_list.git_cmd = 1;

	child_process_def_stdout(&rev_list, STDOUT_PROVISIONED);
	if (pipe(rev_list.out_fd) < 0)
		FATAL("invocation of pipe() system call failed.");

	argv_array_push(&rev_list.args, "rev-list", NULL);
	if (commit_id)
		argv_array_push(&rev_list.args, "--max-count=1", commit_id, NULL);
	else
		argv_array_push(&rev_list.args, "--first-parent", "--no-merges", "HEAD", NULL);

	/*
	 * git-cat-file in batch mode to print commit object for commit ids read from git-rev-list.
	 *
	 * The git-cat-file child process is chained to the git-rev-list child process,
	 * such that the output from git-rev-list is fed into the input of git-cat-file.
	 * With this configuration, we only need to read commit object data from
	 * the cat-file output stream.
	 * */
	child_process_def_init(&cat_file);
	cat_file.git_cmd = 1;

	child_process_def_stdin(&cat_file, STDIN_PROVISIONED);
	cat_file.in_fd[READ] = rev_list.out_fd[READ];
	cat_file.in_fd[WRITE] = rev_list.out_fd[WRITE];

	child_process_def_stdout(&cat_file, STDOUT_PROVISIONED);
	if (pipe(cat_file.out_fd) < 0)
		FATAL("invocation of pipe() system call failed.");

	/* A nonce is generated and used to securely delimit cat-file output.
	 * Without it, specially crafted commit messages could be used to trick
	 * the parser into doing something it's not supposed to.
	 * */
	char nonce[NONCE_LEN];
	char format_arg[] = "--batch=XXXXXXXXXXXXXXXX %(objectname) %(objecttype) %(objectsize)";
	str_template_generate_nonce(format_arg, nonce, NONCE_LEN);
	argv_array_push(&cat_file.args, "cat-file", format_arg, NULL);

	start_command(&rev_list);
	start_command(&cat_file);
	close(cat_file.in_fd[READ]);
	close(cat_file.in_fd[WRITE]);
	close(cat_file.out_fd[WRITE]);

	pager_start();

	do {
		char tmp[BUFF_LEN];

		bytes_read = xread(cat_file.out_fd[READ], tmp, BUFF_LEN);
		if (bytes_read < 0)
			FATAL("failed to read from git-cat-file process");
		if (bytes_read > 0)
			strbuf_attach(&cat_file_out_buf, tmp, bytes_read);

		if (parse_git_cat_file_output(&parsed_commits, &cat_file_out_buf, nonce))
			FATAL("failed to parse batched git-cat-file output");
		for (size_t i = 0; i < parsed_commits.len; i++) {
			struct str_array_entry *entry = str_array_get_entry(&parsed_commits, i);
			struct git_commit *commit = (struct git_commit *) entry->data;

			// attempt to decrypt message
			struct strbuf decrypted_message;
			strbuf_init(&decrypted_message);

			int ret = decrypt_asymmetric_message(&ctx, &commit->body, &decrypted_message);
			if (!ret) {
				// decryption successful
				pretty_print_message(commit, &decrypted_message, 0);
			} else if (ret > 0) {
				// commit body is not gpg message; print commit message body
				pretty_print_message(commit, &commit->body, 1);
			} else {
				strbuf_clear(&decrypted_message);
				strbuf_attach_str(&decrypted_message, "message could not be decrypted.");
				pretty_print_message(commit, &decrypted_message, 2);
			}

			fflush(stdout);

			strbuf_release(&decrypted_message);
			git_commit_object_release(commit);
			free(entry->data);
		}

		str_array_clear(&parsed_commits);
	} while (bytes_read > 0 || cat_file_out_buf.len > 0);

	close(rev_list.out_fd[WRITE]);
	rev_list_exit = finish_command(&rev_list);
	child_process_def_release(&rev_list);

	close(cat_file.out_fd[READ]);
	cat_file_exit = finish_command(&cat_file);
	child_process_def_release(&cat_file);

	strbuf_release(&cat_file_out_buf);
	str_array_release(&parsed_commits);

	if (rev_list_exit)
		return rev_list_exit;
	if (cat_file_exit)
		return cat_file_exit;

	return 0;
}

/**
 * Replace 'X' characters in a null-terminated template string with randomly
 * generated printable hexadecimal digits.
 *
 * If nonce is non-null, it is updated with the characters set in the template
 * string, up to 'len' characters.
 * */
static void str_template_generate_nonce(char str[], char nonce[], size_t len)
{
	struct timeval time;
	if (gettimeofday(&time, NULL))
		FATAL("unable to seed PRNG; gettimeofday failed unexpectedly");

	srandom((unsigned)time.tv_sec ^ (unsigned)time.tv_usec);

	const char *hex_digits = "0123456789abcdef";
	size_t nonce_index = 0;
	for (char *current = strchr(str, 'X'); current != NULL; current = strchr(current, 'X')) {
		*current = hex_digits[random() % 16];

		if (nonce && nonce_index < len) {
			nonce[nonce_index++] = *current;
		}
	}
}

struct object_summary {
	char oid[GIT_HEX_OBJECT_ID];
	long object_len;
	size_t summary_line_len;
};

/**
 * Parse the summary line of batched git-cat-file output for commit id,
 * object length. Verify that the summary line is prefixed with the correct
 * nonce.
 *
 * Returns zero if successful, negative if parsing failed, and positive if
 * not enough data has been read into the buffer.
 * */
static int parse_git_cat_file_output_summary_line(char *output, size_t len,
		struct object_summary *summary, char nonce[NONCE_LEN])
{
	const char *expected_object_type = "commit";

	char *lf = NULL;
	char *commit_id = NULL;
	char *object_type = NULL;
	char *object_len_str = NULL;
	char *tailptr = NULL;
	size_t line_len = 0;
	size_t object_type_str_len = 0;

	// isolate object information line
	lf = memchr(output, '\n', len);
	if (!lf)
		return 1;

	// verify nonce
	line_len = lf - output;
	if (line_len < NONCE_LEN) {
		LOG_ERROR("failed to parse git-cat-file output; line not prefixed with nonce");
		return -1;
	}

	if (memcmp(nonce, output, NONCE_LEN) != 0) {
		LOG_ERROR("failed to parse git-cat-file output; "
				  "invalid nonce, expected '%.*s' but was '%.*s'",
				NONCE_LEN, nonce, NONCE_LEN, output);
		return -1;
	}

	// parse commit id
	commit_id = output + NONCE_LEN + 1;
	if (commit_id >= lf || (lf - commit_id) < GIT_HEX_OBJECT_ID) {
		LOG_ERROR("failed to parse git-cat-file output; no commit id present");
		return -1;
	}

	// parse object type
	object_type = commit_id + GIT_HEX_OBJECT_ID + 1;
	object_type_str_len = strlen(expected_object_type);
	if (object_type >= lf || (size_t)(lf - object_type) < object_type_str_len) {
		LOG_ERROR("failed to parse git-cat-file output; expected object of "
				  "type %s, but was unexpected type", expected_object_type);
		return -1;
	}

	if (memcmp(object_type, expected_object_type, object_type_str_len) != 0) {
		LOG_ERROR("failed to parse git-cat-file output; expected object of "
				  "type %s, but was unexpected type", expected_object_type);
		return -1;
	}

	// parse object length
	object_len_str = object_type + object_type_str_len + 1;
	if (object_len_str >= lf || !(lf - object_len_str)) {
		LOG_ERROR("failed to parse git-cat-file output; unable to parse object length");
		return -1;
	}

	tailptr = NULL;
	long object_len = strtol(object_len_str, &tailptr, 0);
	if (!tailptr || tailptr != lf) {
		LOG_ERROR("failed to parse git-cat-file output; unable to parse object length");
		return -1;
	}

	memcpy(summary->oid, commit_id, GIT_HEX_OBJECT_ID);
	summary->object_len = object_len;
	summary->summary_line_len = lf - output;
	return 0;
}

/**
 * From batched git-cat-file output, attempt to parse a single commit object.
 *
 * Batched git-cat-file output has the format:
 * <commit id> <object type> <object size>
 * <object content>
 *
 * The expected format is quite similar, but introduces a nonce to verify that
 * specially crafted commit messages cannot be used to mislead the parser. The
 * new git-cat-file output has the format:
 * <nonce> <commit id> <object type> <object size>
 * <object content>
 *
 * This function attempts to parse commit objects from batched git-cat-file output,
 * stopping if an object is only partially read, or the parser failed to interpret
 * the output.
 *
 * As commit objects are read from the output buffer, the parsed data is removed,
 * and any unparsed data is left in the buffer.
 *
 * Returns zero if successful, and non-zero if the parser was unable to interpret
 * the data, or critical information couldn't be read from the object summary line.
 * */
static int parse_git_cat_file_output(struct str_array *commits,
		struct strbuf *cat_file_out_buf, char nonce[NONCE_LEN])
{
	do {
		struct object_summary summary;
		int ret = parse_git_cat_file_output_summary_line(cat_file_out_buf->buff,
				cat_file_out_buf->len, &summary, nonce);
		if (ret < 0)
			return 1;
		if (ret > 0)
			break;

		// verify full object exists in buffer
		char *object = cat_file_out_buf->buff + summary.summary_line_len + 1;
		if ((object + summary.object_len + 1) > (cat_file_out_buf->buff + cat_file_out_buf->len))
			break;

		struct git_commit *commit = (struct git_commit *) malloc(sizeof(struct git_commit));
		git_commit_object_init(commit);
		if (commit_parse(commit, summary.oid, object, summary.object_len)) {
			LOG_ERROR("failed to parse commit object from git-cat-file output");
			git_commit_object_release(commit);
			free(commit);
			return 1;
		}

		// shift buffer
		strbuf_remove(cat_file_out_buf, 0, object + summary.object_len - cat_file_out_buf->buff + 1);

		struct str_array_entry *entry = str_array_insert_nodup(commits, NULL, commits->len);
		entry->data = commit;
	} while (1);

	return 0;
}
