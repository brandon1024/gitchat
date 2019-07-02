#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include "str-array.h"
#include "run-command.h"
#include "gpg-interface.h"
#include "fs-utils.h"
#include "usage.h"
#include "utils.h"

#define BUFF_LEN 1024

static const struct usage_description message_cmd_usage[] = {
		USAGE("git chat message [-a | --asym] [(--recipient <alias>...) | (--all)]"),
		USAGE("git chat message (-s | --symmetric) [--password <password>]"),
		USAGE("git chat message (-m | --message) <message>"),
		USAGE("git chat message (-f | --file) <filename>"),
		USAGE("git chat message (-h | --help)"),
		USAGE_END()
};

static const struct option_description message_cmd_options[] = {
		OPT_GROUP("asymmetric (public-key) encryption"),
		OPT_BOOL('a', "asym", "encrypt message using public-key (asymmetric) cryptography"),
		OPT_LONG_STRING("recipient", "alias", "specify one or more recipients that may read the message"),
		OPT_LONG_BOOL("all", "encrypt message with all recipients in the keyring"),

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
static int create_symmetric_message(const char *password, const char *message,
		const char *file);
static void compose_message(struct strbuf *buff);
static void read_message_from_file(const char *file_path, struct strbuf *buff);
static int encrypt_message(struct strbuf *keyring_path, struct strbuf *message,
		struct str_array *recipients, struct strbuf *message_output);
static int write_commit(struct strbuf *encrypted_message);
static void show_message_usage(int err, const char *optional_message_format, ...);


int cmd_message(int argc, char *argv[])
{
	int asym = 0, sym = 0;
	int all_recipients = 0;
	struct str_array recipients;
	const char *message = NULL;
	const char *pass = NULL;
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

		//all recipients
		if (argument_matches_option(arg, message_cmd_options[3])) {
			all_recipients = 1;
			continue;
		}

		//sym
		if (argument_matches_option(arg, message_cmd_options[5])) {
			sym = 1;
			continue;
		}

		//pass
		if (argument_matches_option(arg, message_cmd_options[6])) {
			if (++arg_index >= argc) {
				show_message_usage(1, "error: no password provided with %s", arg);
				str_array_release(&recipients);
				return 1;
			}

			pass = argv[arg_index];
			continue;
		}

		//message
		if (argument_matches_option(arg, message_cmd_options[8])) {
			if (++arg_index >= argc) {
				show_message_usage(1, "error: no message provided with %s", arg);
				str_array_release(&recipients);
				return 1;
			}

			message = argv[arg_index];
			continue;
		}

		//file
		if (argument_matches_option(arg, message_cmd_options[9])) {
			if (++arg_index >= argc) {
				show_message_usage(1, "error: no password provided with %s", arg);
				str_array_release(&recipients);
				return 1;
			}

			file = argv[arg_index];
			continue;
		}

		if (argument_matches_option(arg, message_cmd_options[10])) {
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

	if (asym && pass) {
		show_message_usage(1, "error: --password doesn't make any sense with --asym");
		str_array_release(&recipients);
		return 1;
	}

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

	if (asym && !recipients.len && !all_recipients) {
		show_message_usage(1, "error: at least one recipient must be specified");
		str_array_release(&recipients);
		return 1;
	}

	if (all_recipients)
		str_array_clear(&recipients);

	int ret;
	if (asym)
		ret = create_asymmetric_message(&recipients, message, file);
	else
		ret = create_symmetric_message(pass, message, file);

	str_array_release(&recipients);
	return ret;
}

static int create_asymmetric_message(struct str_array *recipients,
		const char *message, const char *file)
{
	if (!is_inside_git_chat_space())
		DIE("Where are you? It doesn't look like you're in the right directory.");

	struct strbuf message_buff;
	strbuf_init(&message_buff);

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

	struct strbuf keyring_path;
	strbuf_init(&keyring_path);
	strbuf_attach_fmt(&keyring_path, "%s/.git/chat-cache/keyring.gpg", cwd_buff.buff);

	struct strbuf keys_dir_path;
	strbuf_init(&keys_dir_path);
	strbuf_attach_fmt(&keys_dir_path, "%s/.keys", cwd_buff.buff);

	strbuf_release(&cwd_buff);

	LOG_INFO("keyring path: %s", keyring_path.buff);
	if (rebuild_gpg_keyring(keyring_path.buff, keys_dir_path.buff))
		FATAL("failed to rebuild gpg keyring");

	struct strbuf encrypted_message;
	strbuf_init(&encrypted_message);
	if (encrypt_message(&keyring_path, &message_buff, recipients, &encrypted_message))
		FATAL("failed to encrypt message");

	//memset() the plain text message, for security
	memset(message_buff.buff, 0, message_buff.alloc);

	if (write_commit(&encrypted_message))
		FATAL("failed to write to encrypted message as the commit message body");

	strbuf_release(&encrypted_message);
	strbuf_release(&message_buff);
	strbuf_release(&keyring_path);
	strbuf_release(&keys_dir_path);

	return 0;
}

static int create_symmetric_message(const char *password, const char *message,
		const char *file)
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
	strbuf_attach_str(&path_buff, "/.git/COMMIT_EDITMSG");

	const char *msg_compose_file = path_buff.buff;

	//Clear the contents of COMMIT_EDITMSG
	int fd = open(msg_compose_file, O_WRONLY | O_TRUNC);
	if (fd < 0)
		FATAL(FILE_OPEN_FAILED, msg_compose_file);
	close(fd);

	//open vim without backup on COMMIT_EDITMSG
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.executable = "vim";
	argv_array_push(&cmd.args, "-c", "set nobackup nowritebackup", "-n",
			msg_compose_file, NULL);

	status = run_command(&cmd);
	if (status)
		DIE("Vim editor failed with exit status '%d'", status);

	child_process_def_release(&cmd);

	//when vim closes, read COMMIT_EDITMSG into strbuf
	fd = open(msg_compose_file, O_RDONLY);
	if (fd < 0)
		FATAL(FILE_OPEN_FAILED, msg_compose_file);

	while ((bytes_read = read(fd, buffer, BUFF_LEN)) > 0)
		strbuf_attach(buff, buffer, bytes_read);

	if (bytes_read < 0)
		FATAL("Unable to read message from file; failed to read from file descriptor.");

	close(fd);

	//Clear the contents of COMMIT_EDITMSG
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
	int fd = 0;

	int read_stdin = !strcmp(file_path, "-");
	if (!read_stdin) {
		fd = open(file_path, O_RDONLY);
		if (fd < 0)
			FATAL(FILE_OPEN_FAILED, file_path);
	} else
		fprintf(stderr, "[INFO] Type your message below. Once complete, press ⌃D to exit.\n");

	ssize_t bytes_read = 0;
	while ((bytes_read = read(fd, buffer, BUFF_LEN)) > 0)
		strbuf_attach(buff, buffer, bytes_read);

	if (bytes_read < 0)
		FATAL("Unable to read message; read() from file filed");

	if (!read_stdin)
		close(fd);
}

/**
 * Using GPG, encrypt a message in the given strbuf and place the result in the
 * message_output strbuf.
 *
 * The keyring must have the keys for all specified recipients.
 * */
static int encrypt_message(struct strbuf *keyring_path, struct strbuf *message,
		struct str_array *recipients, struct strbuf *message_output)
{
	struct strbuf gpg_executable_path;
	strbuf_init(&gpg_executable_path);
	if (find_gpg_executable(&gpg_executable_path))
		DIE("unable to locate gpg executable");

	// if no recipients, find all recipients
	if (!recipients->len) {
		int ret = retrieve_fingerprints_from_keyring(keyring_path->buff, recipients);
		if (ret < 0)
			FATAL("failed to retrieve fingerprints from keyring %s", keyring_path->buff);

		if (ret == 0)
			DIE("unable to encrypt message; no gpg keys found in keyring");
	}

	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.executable = gpg_executable_path.buff;
	cmd.std_fd_info = STDIN_PROVISIONED | STDOUT_PROVISIONED | STDERR_NULL;

	if (pipe(cmd.in_fd) < 0)
		FATAL("invocation of pipe() system call failed.");
	if (pipe(cmd.out_fd) < 0)
		FATAL("invocation of pipe() system call failed.");

	argv_array_push(&cmd.args, "--no-default-keyring", "--keyring", keyring_path->buff,
			"-ae", "--always-trust", NULL);
	for (size_t i = 0; i < recipients->len; i++)
		argv_array_push(&cmd.args, "-r", recipients->strings[i], NULL);

	if (write(cmd.in_fd[1], message->buff, message->len) != message->len)
		FATAL("failed to write message to pipe");
	close(cmd.in_fd[1]);

	int status = run_command(&cmd);
	child_process_def_release(&cmd);
	strbuf_release(&gpg_executable_path);

	close(cmd.in_fd[0]);
	close(cmd.out_fd[1]);

	char buffer[BUFF_LEN];
	ssize_t bytes_read = 0;
	while ((bytes_read = read(cmd.out_fd[0], buffer, BUFF_LEN)) > 0)
		strbuf_attach(message_output, buffer, bytes_read);

	if (bytes_read < 0)
		FATAL("Unable to read message from file; failed to read from file descriptor.");

	close(cmd.out_fd[0]);
	return status;
}

/**
 * Create a new commit on the tip of the current branch whose commit message body
 * is the encrypted message. Returns the exit status from the git command that
 * was run.
 *
 * Equivalent to running the following command:
 * $ echo $MESSAGE | git commit --allow-empty --file -
 * */
static int write_commit(struct strbuf *encrypted_message)
{
	int status = 0;
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "commit", "--allow-empty", "--file", "-", NULL);

	child_process_def_stdin(&cmd, STDIN_PROVISIONED);
	if (pipe(cmd.in_fd) < 0)
		FATAL("invocation of pipe() system call failed.");

	//write to pipe
	if (write(cmd.in_fd[1], encrypted_message->buff, encrypted_message->len) != encrypted_message->len)
		FATAL("failed to write encrypted message to pipe");
	close(cmd.in_fd[1]);
	status = run_command(&cmd);

	child_process_def_release(&cmd);
	close(cmd.in_fd[0]);

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
