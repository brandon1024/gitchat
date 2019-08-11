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
		USAGE("git chat message (-s | --symmetric)"),
		USAGE("git chat message (-m | --message) <message>"),
		USAGE("git chat message (-f | --file) <filename>"),
		USAGE("git chat message (-h | --help)"),
		USAGE_END()
};

static const struct option_description message_cmd_options[] = {
		OPT_GROUP("asymmetric (public-key) encryption"),
		OPT_BOOL('a', "asym", "encrypt message using public-key (asymmetric) cryptography"),
		OPT_LONG_STRING("recipient", "alias", "specify one or more recipients that may read the message"),

		OPT_GROUP("symmetric (password) encryption"),
		OPT_BOOL('s', "sym", "encrypt the message using private-key (symmetric) cryptography"),
		OPT_LONG_STRING("password", "password", "provide the password necessary to decrypt the message"),

		OPT_GROUP("configuring message"),
		OPT_STRING('m', "message", "message", "provide the message contents"),
		OPT_STRING('f', "file", "filename", "read message contents from file"),
		OPT_BOOL('h', "help", "show usage and exit"),
		OPT_END()
};

static int create_asymmetric_message(struct str_array *recipients,
		const char *message, const char *file);
static int create_symmetric_message(const char *message, const char *file);
static void compose_message(struct strbuf *buff);
static void read_message_from_file(const char *file_path, struct strbuf *buff);
static int encrypt_message(const char *gpg_homedir, struct strbuf *message,
		struct str_array *recipients, struct strbuf *message_output);
static int filter_gpg_keylist_by_recipients(struct _gpgme_key *key, void *data);
static int write_commit(struct strbuf *encrypted_message);
static void show_message_usage(int err, const char *optional_message_format, ...);


int cmd_message(int argc, char *argv[])
{
	int asym = 0, sym = 0;
	struct str_array recipients;
	const char *message = NULL;
	const char *file = NULL;

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

		//message
		if (argument_matches_option(arg, message_cmd_options[6])) {
			if (++arg_index >= argc) {
				show_message_usage(1, "error: no message provided with %s", arg);
				str_array_release(&recipients);
				return 1;
			}

			message = argv[arg_index];
			continue;
		}

		//file
		if (argument_matches_option(arg, message_cmd_options[7])) {
			if (++arg_index >= argc) {
				show_message_usage(1, "error: no password provided with %s", arg);
				str_array_release(&recipients);
				return 1;
			}

			file = argv[arg_index];
			continue;
		}

		if (argument_matches_option(arg, message_cmd_options[8])) {
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

	int ret;
	if (asym)
		ret = create_asymmetric_message(&recipients, message, file);
	else
		ret = create_symmetric_message(message, file);

	str_array_release(&recipients);
	return ret;
}

static int create_asymmetric_message(struct str_array *recipients,
		const char *message, const char *file)
{
	struct strbuf gpg_homedir;
	struct strbuf keys_dir_path;

	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	struct strbuf message_buff;
	strbuf_init(&message_buff);

	// build the message into the message_buff buffer
	if (!message && !file)
		compose_message(&message_buff);
	else if (file)
		read_message_from_file(file, &message_buff);
	else
		strbuf_attach_str(&message_buff, message);

	struct strbuf cwd_buff;
	strbuf_init(&cwd_buff);
	if (get_cwd(&cwd_buff))
		FATAL("unable to obtain the current working directory from getcwd()");

	strbuf_init(&gpg_homedir);
	strbuf_init(&keys_dir_path);

	strbuf_attach_fmt(&gpg_homedir, "%s/.git/.gnupg", cwd_buff.buff);
	strbuf_attach_fmt(&keys_dir_path, "%s/.keys", cwd_buff.buff);
	LOG_INFO("gpg homedir %s", gpg_homedir.buff);
	LOG_INFO("git-chat keys dir %s", keys_dir_path.buff);

	strbuf_release(&cwd_buff);

	// reimport the gpg keys into the keyring
	rebuild_gpg_keyring(gpg_homedir.buff, keys_dir_path.buff);
	strbuf_release(&keys_dir_path);

	// encrypt the message into the encrypted_message buffer
	struct strbuf encrypted_message;
	strbuf_init(&encrypted_message);
	if (encrypt_message(gpg_homedir.buff, &message_buff, recipients, &encrypted_message)) {
		WARN("one or more recipients have no known GPG keys in the keyring.");

		strbuf_release(&gpg_homedir);
		strbuf_release(&message_buff);
		strbuf_release(&encrypted_message);
		return 1;
	}

	strbuf_release(&gpg_homedir);

	memset(message_buff.buff, 0, message_buff.alloc);
	strbuf_release(&message_buff);

	// write the commit onto the tip of the branch
	if (write_commit(&encrypted_message))
		FATAL("failed to write ciphertext as the commit message body");

	strbuf_release(&encrypted_message);

	return 0;
}

static int create_symmetric_message(const char *message, const char *file)
{
	return 1;
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
			FATAL(FILE_OPEN_FAILED, file_path);
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
 * Encrypt a plaintext message from a string buffer into destination buffer, in
 * ASCII-armor format.
 *
 * If recipients is an empty list, then all gpg keys are used in encrypting the
 * message. Otherwise, recipients are mapped to gpg keys using the
 * filter_gpg_keylist_by_recipients() filter function. If one or more recipients
 * do not have associated GPG keys, returns 1 and the message output buffer is left
 * unmodified.
 *
 * If the message was successfully encrypted into the output buffer, returns zero.
 * */
static int encrypt_message(const char *gpg_homedir, struct strbuf *message,
		struct str_array *recipients, struct strbuf *message_output)
{
	struct gpg_key_list gpg_keys;
	int key_count = get_gpg_keys_from_keyring(gpg_homedir, &gpg_keys);

	// filter unusable and secret gpg keys
	key_count -= filter_gpg_keys_by_predicate(&gpg_keys, filter_gpg_unusable_keys, NULL);
	key_count -= filter_gpg_keys_by_predicate(&gpg_keys, filter_gpg_secret_keys, NULL);

	if (!key_count)
		DIE("no message recipients; no one will be able to read your message.");

	// if explicit recipients given, filter keys that are not to be recipients
	if (recipients->len) {
		key_count -= filter_gpg_keys_by_predicate(&gpg_keys,
				filter_gpg_keylist_by_recipients, recipients);

		// if there is not a 1-1 mapping of recipients to gpg keys, fail
		if (key_count != recipients->len) {
			LOG_ERROR("Some recipients defined cannot be mapped to GPG keys");

			release_gpg_key_list(&gpg_keys);
			return 1;
		}
	}

	encrypt_plaintext_message(gpg_homedir, message, message_output, &gpg_keys);
	release_gpg_key_list(&gpg_keys);

	return 0;
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
	argv_array_push(&cmd.args, "show", "--format=%C(yellow)commit %h%Creset %d%nauthor: %aE",
			"-s", "HEAD", NULL);
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
