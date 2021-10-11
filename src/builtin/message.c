#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "str-array.h"
#include "run-command.h"
#include "git/graph-traversal.h"
#include "gnupg/gpg-common.h"
#include "gnupg/key-trust.h"
#include "gnupg/encryption.h"
#include "gnupg/decryption.h"
#include "gnupg/key-filter.h"
#include "gnupg/key-manager.h"
#include "working-tree.h"
#include "fs-utils.h"
#include "parse-options.h"
#include "utils.h"

#define BUFF_LEN 1024

static const struct usage_string message_cmd_usage[] = {
		USAGE("git chat message [(--recipient <alias>)...] [(--reply | --compose <n>)]"),
		USAGE("git chat message [(--recipient <alias>)...] (-m | --message) <message>"),
		USAGE("git chat message [(--recipient <alias>)...] (-f | --file) <filename>"),
		USAGE("git chat message (-h | --help)"),
		USAGE_END()
};

struct graph_traversal_context {
	int message_fd;
	struct gc_gpgme_ctx *gpg_ctx;
};

/**
 * Launch vim to edit the file `compose_file`, optionally opening the file
 * `ro_info_pane_file` in a separate read-only window for informational messages.
 *
 * Returns the exit status of vim.
 * */
static int launch_editor(const char *compose_file, const char *ro_info_pane_file)
{
	struct strbuf info_panel_op, compose_panel_op;
	struct child_process_def cmd;
	int ret;

	strbuf_init(&info_panel_op);
	strbuf_init(&compose_panel_op);

	child_process_def_init(&cmd);
	cmd.executable = "vim";

	argv_array_push(&cmd.args, "-n", "-c", "set nobackup nowritebackup", NULL);

	// open `ro_info_pane_file` in a read-only split panel
	if (ro_info_pane_file) {
		strbuf_attach_fmt(&info_panel_op, "view %s", ro_info_pane_file);
		strbuf_attach_fmt(&compose_panel_op, "abo sp %s", compose_file);

		argv_array_push(&cmd.args, "-c", info_panel_op.buff, "-c",
				compose_panel_op.buff, NULL);
	} else {
		argv_array_push(&cmd.args, compose_file, NULL);
	}

	ret = run_command(&cmd);

	child_process_def_release(&cmd);
	strbuf_release(&info_panel_op);
	strbuf_release(&compose_panel_op);

	return ret;
}

/**
 * Create a new file `file` with rw permissions for the current user, or
 * truncate the file if it already exists.
 * */
static void create_truncate_file(const char *file)
{
	int fd = open(file, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0)
		FATAL(FILE_OPEN_FAILED, file);
	close(fd);
}

/**
 * Callback function invoked by the `graph-traversal` API. Attempts to decrypt
 * messages from the given `commit` and write a pretty-printed result to
 * a file descriptor supplied through `data`.
 *
 * The void pointer is assumed to be a pointer to a `graph_traversal_context`
 * structure.
 *
 * Returns zero to indicate success.
 * */
static int commit_traversal_cb(struct git_commit *commit, void *data)
{
	struct graph_traversal_context *ctx = (struct graph_traversal_context *) data;
	struct gc_gpgme_ctx *gpg_ctx = ctx->gpg_ctx;
	int fd = ctx->message_fd;

	struct strbuf decrypted_text;
	strbuf_init(&decrypted_text);

	int ret = decrypt_asymmetric_message(gpg_ctx, &commit->body, &decrypted_text);
	if (!ret) {
		// decryption successful
		pretty_print_message(commit, &decrypted_text, DECRYPTED, 1, fd);
	} else if (ret > 0) {
		// commit body is not gpg message; print commit message body
		pretty_print_message(commit, &commit->body, PLAINTEXT, 1, fd);
	} else {
		strbuf_clear(&decrypted_text);
		strbuf_attach_str(&decrypted_text, "message could not be decrypted.");
		pretty_print_message(commit, &decrypted_text, UNKNOWN_ERROR, 1, fd);
	}

	strbuf_release(&decrypted_text);

	return 0;
}

/**
 * Prompt the user with a vim editor to compose their message. Upon editing,
 * the message is attached to the given strbuf `buff`.
 *
 * When `compose` is non-zero, that number of messages on the current channel
 * will be decrypted and shown in the editor.
 *
 * When vim is started, the editor will open `.git/GC_EDITMSG` with rw permission
 * for the current user only. The file is first truncated to zero bytes, then
 * vim is opened to write to the file. Once vim exits, the file is read into the
 * given buffer, and then the file is truncated once more.
 *
 * Decrypted messages are written to `.git/GC_REPLY_LAST` with rw permission for
 * the current user only. Once vim exits, this file is truncated.
 * */
static void compose_message(struct strbuf *buff, int compose)
{
	struct strbuf cwd, compose_file;

	if (!isatty(STDOUT_FILENO))
		DIE("cannot launch editor; output is not a terminal");

	strbuf_init(&cwd);
	strbuf_init(&compose_file);

	if (get_cwd(&cwd))
		FATAL("unable to obtain the current working directory from getcwd()");
	strbuf_attach_fmt(&compose_file, "%s/.git/GC_EDITMSG", cwd.buff);

	// create or clear files with correct permissions
	create_truncate_file(compose_file.buff);

	int ret;
	if (compose) {
		struct gc_gpgme_ctx gpg_ctx;
		gpgme_context_init(&gpg_ctx, 0);

		struct strbuf reply_messages_file;
		strbuf_init(&reply_messages_file);
		strbuf_attach_fmt(&reply_messages_file, "%s/.git/GC_REPLY_LAST", cwd.buff);

		int fd = open(reply_messages_file.buff, O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR);
		if (fd < 0)
			FATAL(FILE_OPEN_FAILED, reply_messages_file.buff);

		INFO("decrypting messages, this may take a few seconds");

		struct graph_traversal_context cb_ctx = { .gpg_ctx = &gpg_ctx, .message_fd = fd };
		ret = traverse_commit_graph(NULL, compose, commit_traversal_cb, &cb_ctx);
		if (ret)
			FATAL("commit graph traversal failed");

		close(fd);

		ret = launch_editor(compose_file.buff, reply_messages_file.buff);

		create_truncate_file(reply_messages_file.buff);
		strbuf_release(&reply_messages_file);

		gpgme_context_release(&gpg_ctx);
	} else {
		ret = launch_editor(compose_file.buff, NULL);
	}

	if (ret)
		DIE("vim exited unexpectedly (status %d)", ret);

	int fd = open(compose_file.buff, O_RDONLY);
	if (fd < 0)
		FATAL(FILE_OPEN_FAILED, compose_file.buff);

	strbuf_attach_fd(buff, fd);
	close(fd);

	create_truncate_file(compose_file.buff);
	strbuf_release(&compose_file);
	strbuf_release(&cwd);
}

/**
 * Read the contents of a given file into the strbuf. If file_path is "-",
 * read from stdin instead.
 * */
static void read_message_from_file(const char *file_path, struct strbuf *buff)
{
	int fd = STDIN_FILENO;

	int read_stdin = !strcmp(file_path, "-");
	if (!read_stdin) {
		fd = open(file_path, O_RDONLY);
		if (fd < 0)
			DIE(FILE_OPEN_FAILED, file_path);
	} else
		fprintf(stderr, "[INFO] Type your message below. Once complete, press ⌃D to exit.\n");

	strbuf_attach_fd(buff, fd);

	if (!read_stdin)
		close(fd);
}

/**
 * Filter function used by encrypt_message(...) to filter any keys that are not
 * specified in a recipients str_array (given as the data argument).
 *
 * Keys are filtered if:
 * - no recipients match the primary key fingerprint
 * - no recipients match any subkey uid field
 * - no recipients match any subkey name field
 * - no recipients match any subkey email field
 * - no recipients match any subkey comment field
 * - no recipients match any subkey address field
 * */
static int filter_gpg_keylist_by_recipients(gpgme_key_t key, void *data)
{
	struct str_array *recipients = (struct str_array *)data;

	for (size_t index = 0; index < recipients->len; index++) {
		const char *recipient = str_array_get(recipients, index);
		if (key->fpr && !strcmp(key->fpr, recipient))
			return 1;

		struct _gpgme_user_id *uid = key->uids;
		while (uid) {
			if (uid->uid && !strcmp(uid->uid, recipient))
				return 1;
			if (uid->name && !strcmp(uid->name, recipient))
				return 1;
			if (uid->email && !strcmp(uid->email, recipient))
				return 1;
			if (uid->comment && !strcmp(uid->comment, recipient))
				return 1;
			if (uid->address && !strcmp(uid->address, recipient))
				return 1;

			uid = uid->next;
		}
	}

	return 0;
}

/**
 * Key list filter predicate that is identical to the
 * `filter_gpg_keys_by_fingerprint` but logs a message at INFO level indicating
 * that the key was filtered because the fingerprint didn't exist in the trusted
 * keys list.
 * */
static int filter_gpg_keys_by_fingerprint_verbose(gpgme_key_t key, void *data)
{
	if (!filter_gpg_keys_by_fingerprint(key, data)) {
		LOG_INFO("recipient with fingerprint '%s' filtered by the trust keys list",
				key->fpr);
		return 0;
	}

	return 1;
}

/**
 * Encrypt a plaintext message from a string buffer into destination buffer, in
 * ASCII-armor format.
 *
 * If recipients is an empty list, then all gpg keys are used in encrypting the
 * message. Otherwise, recipients are mapped to gpg keys using the
 * filter_gpg_keylist_by_recipients() filter function. If one or more recipients
 * do not have associated GPG keys, returns 1 and the message output buffer is left
 * unmodified.
 *
 * Returns the number of recipients selected to decrypt the message, or -1 if
 * some recipients in the given str_array do not exist (no public key in keyring).
 * */
static int encrypt_message_asym(struct gc_gpgme_ctx *ctx, struct str_array *recipients,
		struct strbuf *message_in, struct strbuf *ciphertext_result)
{
	struct gpg_key_list gpg_keys;
	int key_count = fetch_gpg_keys(ctx, &gpg_keys);

	// filter unusable and secret gpg keys
	key_count -= filter_gpg_keys_by_predicate(&gpg_keys, filter_gpg_unusable_keys, NULL);
	key_count -= filter_gpg_keys_by_predicate(&gpg_keys, filter_gpg_secret_keys, NULL);

	if (recipients->len) {
		// if explicit recipients given, filter keys that are not to be recipients
		key_count -= filter_gpg_keys_by_predicate(&gpg_keys,
				filter_gpg_keylist_by_recipients, recipients);

		// if there is not a 1-1 mapping of recipients to gpg keys, fail
		if ((size_t)key_count != recipients->len) {
			LOG_ERROR("some recipients defined cannot be mapped to GPG keys");

			release_gpg_key_list(&gpg_keys);
			return -1;
		}
	}

	// filter by trusted keys
	struct str_array trust_list;
	str_array_init(&trust_list);

	if (read_trust_list(&trust_list) >= 0)
		key_count -= filter_gpg_keys_by_predicate(&gpg_keys,
				filter_gpg_keys_by_fingerprint_verbose, (void *) &trust_list);

	str_array_release(&trust_list);

	if (key_count)
		asymmetric_encrypt_plaintext_message(ctx, message_in, ciphertext_result, &gpg_keys);

	release_gpg_key_list(&gpg_keys);

	return key_count;
}

/**
 * Create a new commit on the tip of the current branch whose commit message body
 * is the encrypted message. Returns the exit status from the git command that
 * was run.
 *
 * Equivalent to running the following command:
 * $ echo $MESSAGE | git commit -q --allow-empty --file -
 * */
static int write_commit(struct strbuf *encrypted_message)
{
	struct child_process_def cmd;

	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "commit", "--no-gpg-sign", "--allow-empty",
			"--file", "-", NULL);

	child_process_def_stdin(&cmd, STDIN_PROVISIONED);
	child_process_def_stdout(&cmd, STDOUT_NULL);
	if (pipe(cmd.in_fd) < 0)
		FATAL("invocation of pipe() system call failed.");

	start_command(&cmd);

	close(cmd.in_fd[0]);
	if (xwrite(cmd.in_fd[1], encrypted_message->buff, encrypted_message->len) != (ssize_t)encrypted_message->len)
		FATAL("failed to write encrypted message to pipe");
	close(cmd.in_fd[1]);

	int status = finish_command(&cmd);
	child_process_def_release(&cmd);
	if (status)
		return 1;

	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "--no-pager", "show", "-s", "--format=%B", "HEAD", NULL);
	status = run_command(&cmd);

	child_process_def_release(&cmd);

	return status;
}

/**
 * Create a new message with the given recipients.
 *
 * The mutually-exclusive arguments `message` and `file` are used to supply
 * the contents of the message. If both arguments are NULL, the editor
 * is spawned to allow the user to compose the message.
 *
 * If `file` is non-null, the message content is read from a file with the
 * given path, unless the path is `-` which will read the message from stdin.
 *
 * When `compose` is non-zero, that number of messages on the current channel
 * will be decrypted and shown in the editor. This will only take effect if both
 * `file` and `message` arguments are NULL.
 *
 * Message cannot be empty.
 * */
static int create_message(struct str_array *recipients, const char *message,
		const char *file, int compose)
{
	struct gc_gpgme_ctx ctx;
	struct strbuf message_buff, ciphertext, keys_dir_path;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	strbuf_init(&message_buff);

	// build the message into the message_buff buffer
	if (!message && !file)
		compose_message(&message_buff, compose);
	else if (file)
		read_message_from_file(file, &message_buff);
	else
		strbuf_attach_str(&message_buff, message);

	if (!message_buff.len)
		DIE("message aborted (message was not provided)");

	strbuf_init(&keys_dir_path);
	if (get_keys_dir(&keys_dir_path))
		FATAL(".keys directory does not exist or cannot be used for some reason");

	gpgme_context_init(&ctx, 1);
	strbuf_init(&ciphertext);

	// reimport the gpg keys into the keyring
	rebuild_gpg_keyring(&ctx, keys_dir_path.buff);
	strbuf_release(&keys_dir_path);

	int ret = encrypt_message_asym(&ctx, recipients, &message_buff, &ciphertext);
	if (ret == 0)
		DIE("no message recipients; no one will be able to read your message.");
	if (ret < 0)
		DIE("one or more message recipients have no public gpg key available.");

	gpgme_context_release(&ctx);

	memset(message_buff.buff, 0, message_buff.alloc);

	if (write_commit(&ciphertext))
		FATAL("failed to write ciphertext as the commit message body");

	strbuf_release(&message_buff);
	strbuf_release(&ciphertext);

	return 0;
}

int cmd_message(int argc, char *argv[])
{
	int show_help = 0;
	int reply = 0, compose = 0;
	struct str_array recipients;
	char *message = NULL;
	char *file = NULL;

	const struct command_option message_cmd_options[] = {
			OPT_LONG_STRING_LIST("recipient", "alias", "specify one or more recipients that may read the message", &recipients),
			OPT_STRING('m', "message", "message", "provide the message contents", &message),
			OPT_STRING('f', "file", "filename", "read message contents from file", &file),
			OPT_LONG_BOOL("reply", "show the last message when composing new messages", &reply),
			OPT_LONG_INT("compose", "show last messages when composing new messages", &compose),
			OPT_BOOL('h', "help", "show usage and exit", &show_help),
			OPT_END()
	};

	str_array_init(&recipients);
	argc = parse_options(argc, argv, message_cmd_options, 1, 1);
	if (argc > 0) {
		show_usage_with_options(message_cmd_usage, message_cmd_options, 1,
				"error: unknown option '%s'", argv[0]);
		str_array_release(&recipients);
		return 1;
	}

	if (show_help) {
		show_usage_with_options(message_cmd_usage, message_cmd_options, 0, NULL);
		str_array_release(&recipients);
		return 0;
	}

	if (message && file) {
		show_usage_with_options(message_cmd_usage, message_cmd_options, 1,
				"error: mixing --message and --file is not supported");
		str_array_release(&recipients);
		return 1;
	}

	if (compose < 0) {
		show_usage_with_options(message_cmd_usage, message_cmd_options, 1,
				"error: --reply-last value must be non-negative");
		str_array_release(&recipients);
		return 1;
	}

	// reply-last option takes precedence
	compose = (compose > 0) ? compose : reply;

	int ret = create_message(&recipients, message, file, compose);

	str_array_release(&recipients);
	return ret;
}
