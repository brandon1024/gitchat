#ifndef GIT_CHAT_COMMIT_H
#define GIT_CHAT_COMMIT_H

#include <inttypes.h>

#include "strbuf.h"
#include "git/git.h"

struct git_time {
	/**
	 * time in seconds from epoch
	 * */
	int64_t time;

	/**
	 * timezone offset, in minutes
	 * */
	int offset;
};

struct git_signature {
	struct strbuf name;
	struct strbuf email;
	struct git_time timestamp;
};

struct git_commit {
	struct git_oid commit_id;
	struct git_oid tree_id;
	struct git_oid *parents_commit_ids;
	size_t parents_commit_ids_len;

	struct git_signature author;
	struct git_signature committer;

	struct strbuf body;
};

enum message_type {
	PLAINTEXT,
	DECRYPTED,
	UNKNOWN_ERROR
};

/**
 * Allocate memory and initialize members of a commit object to their defaults.
 * */
void git_commit_object_init(struct git_commit *commit);

/**
 * Release any memory under the commit objects and restore defaults.
 * */
void git_commit_object_release(struct git_commit *commit);

/**
 * Commit to the git tree any files currently tracked under the git index.
 * The commit will have the given commit message. This is equivalent to running
 * the following:
 * git commit -m <message>
 * */
int git_commit_index(const char *commit_message);

/**
 * Similar to git_commit_index(), create a new commit with a given commit message
 * and with optional command-line arguments.
 *
 * This is equivalent to running the following:
 * git commit -m <message> <args>...
 * */
__attribute__ ((sentinel))
int git_commit_index_with_options(const char *commit_message, ...);

/**
 * Attempts to parse a buffer containing a raw commit object (as given by git-cat-file).
 *
 * Returns zero if successful, non-zero otherwise.
 * */
int commit_parse(struct git_commit *commit, const char commit_id[GIT_HEX_OBJECT_ID],
		const char *data, size_t len);

/**
 * Pretty-print a single message and write to the file descriptor `output_fd`.
 *
 * `commit` is the raw commit, as read from `commit_parse()`.
 *
 * `message` represents a raw or externally-decrypted message. `message_type`
 * is used to indicate the format of the message, either:
 * - plain text message,
 * - decrypted message, or
 * - the raw message when decryption fails unexpectedly.
 *
 * `no_color` is used to control whether raw ANSI color control sequences are
 * written to the output.
 *
 * Messages are displayed in the following format:
 * [<timestamp> <ENC | PLN> <author>]
 *   <message>...
 *   <message>...
 * */
void pretty_print_message(struct git_commit *commit, struct strbuf *message,
		enum message_type type, int no_color, int output_fd);

#endif //GIT_CHAT_COMMIT_H
