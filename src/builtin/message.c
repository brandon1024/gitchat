#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "str-array.h"
#include "run-command.h"
#include "gpg-interface.h"
#include "fs-utils.h"
#include "usage.h"
#include "utils.h"

#define BUFF_LEN 1024

static const struct usage_description message_cmd_usage[] = {
		USAGE("git chat message [-a | --asym] [(--recipient <alias>...)]"),
		USAGE("git chat message (-s | --symmetric) [--passphrase <passphrase>]"),
		USAGE("git chat message (-m | --message) <message>"),
		USAGE("git chat message (-f | --file) <filename>"),
		USAGE("git chat message (-h | --help)"),
		USAGE_END()
};

static const struct option_description message_cmd_options[] = {
		OPT_GROUP("asymmetric (public-key) encryption"),
		OPT_BOOL('a', "asym", "encrypt message using public-key (asymmetric) cryptography"),
		OPT_LONG_STRING("recipient", "alias", "specify one or more recipients that may read the message"),

		OPT_GROUP("symmetric (password-based) encryption"),
		OPT_BOOL('s', "sym", "encrypt the message using passphrase-based (symmetric) cryptography"),
		OPT_LONG_STRING("passphrase", "passphrase", "skip the default pinentry method and provide passphrase as argument"),

		OPT_GROUP("configuring message"),
		OPT_STRING('m', "message", "message", "provide the message contents"),
		OPT_STRING('f', "file", "filename", "read message contents from file"),
		OPT_BOOL('h', "help", "show usage and exit"),
		OPT_END()
};

static int create_message(struct str_array *recipients, const char *message,
		const char *file, const char *passphrase, int asym);
static int encrypt_message_asym(const char *gpg_homedir, struct str_array *recipients,
		struct strbuf *message_in, struct strbuf *ciphertext_result);
static void compose_message(struct strbuf *buff);
static void read_message_from_file(const char *file_path, struct strbuf *buff);
static int filter_gpg_keylist_by_recipients(struct _gpgme_key *key, void *data);
static int write_commit(struct strbuf *encrypted_message);
static void show_message_usage(int err, const char *optional_message_format, ...);


int cmd_message(int argc, char *argv[])
{
	int asym = 0, sym = 0;
	struct str_array recipients;
	const char *message = NULL;
	const char *file = NULL;
	const char *passphrase = NULL;

	str_array_init(&recipients);

	for (size_t arg_index = 0; arg_index < argc; arg_index++) {
		char *arg = argv[arg_index];

		if (!is_valid_argument(arg, message_cmd_options)) {
			show_message_usage(1, "error: unknown option '%s'", arg);
			return 1;
		}

		//asym
		if (argument_matches_option(arg, message_cmd_options[1])) {
			asym = 1;
			continue;
		}

		//recipients
		if (argument_matches_option(arg, message_cmd_options[2])) {
			while ((arg_index + 1) < argc) {
				arg = argv[arg_index + 1];
				if (arg[0] == '-')
					break;

				str_array_push(&recipients, arg, NULL);
				arg_index++;
			}

			continue;
		}

		//sym
		if (argument_matches_option(arg, message_cmd_options[4])) {
			sym = 1;
			continue;
		}

		//passphrase
		if (argument_matches_option(arg, message_cmd_options[5])) {
			if (++arg_index >= argc) {
				show_message_usage(1, "error: no passphrase provided with %s", arg);
				str_array_release(&recipients);
				return 1;
			}

			passphrase = argv[arg_index];
			continue;
		}

		//message
		if (argument_matches_option(arg, message_cmd_options[7])) {
			if (++arg_index >= argc) {
				show_message_usage(1, "error: no message provided with %s", arg);
				str_array_release(&recipients);
				return 1;
			}

			message = argv[arg_index];
			continue;
		}

		//file
		if (argument_matches_option(arg, message_cmd_options[8])) {
			if (++arg_index >= argc) {
				show_message_usage(1, "error: no file provided with %s", arg);
				str_array_release(&recipients);
				return 1;
			}

			file = argv[arg_index];
			continue;
		}

		//show help
		if (argument_matches_option(arg, message_cmd_options[9])) {
			show_message_usage(0, NULL);
			str_array_release(&recipients);
			return 0;
		}
	}

	if (sym && asym) {
		show_message_usage(1, "error: cannot combine --sym and --asym");
		str_array_release(&recipients);
		return 1;
	}

	//prefer asym
	if (!sym && !asym)
		asym = 1;

	if (sym && recipients.len) {
		show_message_usage(1, "error: --recipient doesn't make any sense with --sym");
		str_array_release(&recipients);
		return 1;
	}

	if (message && file) {
		show_message_usage(1, "error: mixing --message and --file is not supported");
		str_array_release(&recipients);
		return 1;
	}

	if (passphrase && asym) {
		show_message_usage(1, "error: --passphrase doesn't make sense with asymmetric encryption");
		str_array_release(&recipients);
		return 1;
	}

	int ret = create_message(&recipients, message, file, passphrase, asym);

	str_array_release(&recipients);
	return ret;
}

static int create_message(struct str_array *recipients, const char *message,
		const char *file, const char *passphrase, const int asym)
{
	struct strbuf gpg_homedir;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	struct strbuf cwd_buff;
	strbuf_init(&cwd_buff);
	if (get_cwd(&cwd_buff))
		FATAL("unable to obtain the current working directory from getcwd()");

	strbuf_init(&gpg_homedir);
	strbuf_attach_fmt(&gpg_homedir, "%s/.git/.gnupg", cwd_buff.buff);
	LOG_INFO("gpg homedir %s", gpg_homedir.buff);

	struct strbuf message_buff;
	strbuf_init(&message_buff);

	// build the message into the message_buff buffer
	if (!message && !file)
		compose_message(&message_buff);
	else if (file)
		read_message_from_file(file, &message_buff);
	else
		strbuf_attach_str(&message_buff, message);

	struct strbuf ciphertext;
	strbuf_init(&ciphertext);
	if (asym) {
		struct strbuf keys_dir_path;
		strbuf_init(&keys_dir_path);
		strbuf_attach_fmt(&keys_dir_path, "%s/.keys", cwd_buff.buff);
		LOG_INFO("git-chat keys dir %s", keys_dir_path.buff);

		// reimport the gpg keys into the keyring
		rebuild_gpg_keyring(gpg_homedir.buff, keys_dir_path.buff);
		strbuf_release(&keys_dir_path);

		int ret = encrypt_message_asym(gpg_homedir.buff, recipients,
				&message_buff, &ciphertext);
		if (ret == 0)
			DIE("no message recipients; no one will be able to read your message.");
		if (ret < 0)
			DIE("one or more message recipients have no public gpg key available.");
	} else {
		symmetric_encrypt_plaintext_message(gpg_homedir.buff, &message_buff,
				&ciphertext, passphrase);
	}

	memset(message_buff.buff, 0, message_buff.alloc);

	if (write_commit(&ciphertext))
		FATAL("failed to write ciphertext as the commit message body");

	strbuf_release(&message_buff);
	strbuf_release(&ciphertext);
	strbuf_release(&cwd_buff);
	strbuf_release(&gpg_homedir);

	return 0;
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
static int encrypt_message_asym(const char *gpg_homedir, struct str_array *recipients,
		struct strbuf *message_in, struct strbuf *ciphertext_result)
{
	struct gpg_key_list gpg_keys;
	int key_count = get_gpg_keys_from_keyring(gpg_homedir, &gpg_keys);

	// filter unusable and secret gpg keys
	key_count -= filter_gpg_keys_by_predicate(&gpg_keys, filter_gpg_unusable_keys, NULL);
	key_count -= filter_gpg_keys_by_predicate(&gpg_keys, filter_gpg_secret_keys, NULL);

	// if explicit recipients given, filter keys that are not to be recipients
	if (recipients->len) {
		key_count -= filter_gpg_keys_by_predicate(&gpg_keys,
				filter_gpg_keylist_by_recipients, recipients);

		// if there is not a 1-1 mapping of recipients to gpg keys, fail
		if (key_count != recipients->len) {
			LOG_ERROR("Some recipients defined cannot be mapped to GPG keys");

			release_gpg_key_list(&gpg_keys);
			return -1;
		}
	}

	if (key_count)
		asymmetric_encrypt_plaintext_message(gpg_homedir, message_in, ciphertext_result, &gpg_keys);

	release_gpg_key_list(&gpg_keys);

	return key_count;
}

/**
 * Allow the user to compose a message by running vim as a child process
 * and reading the contents of the file into the given strbuf.
 *
 * This function does not use temporary files. Instead, it uses .git/COMMIT_EDITMSG.
 * The file is first truncated to zero bytes, then vim is opened to write to the file.
 * Once vim exits, the file is read into the given buffer, and then the file is truncated
 * once more.
 * */
static void compose_message(struct strbuf *buff)
{
	char buffer[BUFF_LEN];
	ssize_t bytes_read = 0;
	int status;

	struct strbuf path_buff;
	strbuf_init(&path_buff);
	if (get_cwd(&path_buff))
		FATAL("unable to obtain the current working directory from getcwd()");
	strbuf_attach_str(&path_buff, "/.git/GC_EDITMSG");

	const char *msg_compose_file = path_buff.buff;

	// clear the contents of GC_EDITMSG
	int fd = open(msg_compose_file, O_WRONLY | O_TRUNC);
	if (fd < 0)
		FATAL(FILE_OPEN_FAILED, msg_compose_file);
	close(fd);

	// open vim without backup on GC_EDITMSG
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.executable = "vim";
	argv_array_push(&cmd.args, "-c", "set nobackup nowritebackup", "-n",
			msg_compose_file, NULL);

	status = run_command(&cmd);
	if (status)
		DIE("Vim editor failed with exit status '%d'", status);

	child_process_def_release(&cmd);

	// when vim closes, read GC_EDITMSG into strbuf
	fd = open(msg_compose_file, O_RDONLY);
	if (fd < 0)
		FATAL(FILE_OPEN_FAILED, msg_compose_file);

	while ((bytes_read = recoverable_read(fd, buffer, BUFF_LEN)) > 0)
		strbuf_attach(buff, buffer, bytes_read);

	if (bytes_read < 0)
		FATAL("Unable to read message from file; failed to read from file descriptor.");

	close(fd);

	// clear the contents of GC_EDITMSG
	fd = open(msg_compose_file, O_WRONLY | O_TRUNC);
	if (fd < 0)
		FATAL(FILE_OPEN_FAILED, msg_compose_file);

	close(fd);
	strbuf_release(&path_buff);
}

/**
 * Read the contents of a given file into the strbuf. If file_path is "-",
 * read from stdin instead.
 * */
static void read_message_from_file(const char *file_path, struct strbuf *buff)
{
	char buffer[BUFF_LEN];
	int fd = STDIN_FILENO;

	int read_stdin = !strcmp(file_path, "-");
	if (!read_stdin) {
		fd = open(file_path, O_RDONLY);
		if (fd < 0)
			DIE(FILE_OPEN_FAILED, file_path);
	} else
		fprintf(stderr, "[INFO] Type your message below. Once complete, press ⌃D to exit.\n");

	ssize_t bytes_read = 0;
	while ((bytes_read = recoverable_read(fd, buffer, BUFF_LEN)) > 0)
		strbuf_attach(buff, buffer, bytes_read);

	if (bytes_read < 0)
		FATAL("Unable to read message; read() from file filed");

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
static int filter_gpg_keylist_by_recipients(struct _gpgme_key *key, void *data)
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
 * Create a new commit on the tip of the current branch whose commit message body
 * is the encrypted message. Returns the exit status from the git command that
 * was run.
 *
 * Equivalent to running the following command:
 * $ echo $MESSAGE | git commit -q --allow-empty --file -
 * */
static int write_commit(struct strbuf *encrypted_message)
{
	int status = 0;
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
	if (recoverable_write(cmd.in_fd[1], encrypted_message->buff, encrypted_message->len) != encrypted_message->len)
		FATAL("failed to write encrypted message to pipe");
	close(cmd.in_fd[1]);

	status = finish_command(&cmd);
	child_process_def_release(&cmd);
	if (status)
		return 1;

	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "cat-file", "-p", "HEAD", NULL);
	status = run_command(&cmd);

	child_process_def_release(&cmd);

	return status;
}

static void show_message_usage(int err, const char *optional_message_format, ...)
{
	va_list varargs;
	va_start(varargs, optional_message_format);

	variadic_show_usage_with_options(message_cmd_usage, message_cmd_options,
			optional_message_format, varargs, err);

	va_end(varargs);
}
