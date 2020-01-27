#include <stdarg.h>

#include "git.h"
#include "run-command.h"

int git_add_file_to_index(const char *file)
{
	struct str_array files;
	str_array_init(&files);
	str_array_push(&files, file, NULL);

	int ret = git_add_files_to_index(&files);
	str_array_release(&files);

	return ret;
}

int git_add_files_to_index(struct str_array *file_paths)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.std_fd_info = STDIN_NULL | STDOUT_NULL | STDERR_NULL;

	argv_array_push(&cmd.args, "add", NULL);

	for (size_t i = 0; i < file_paths->len; i++)
		argv_array_push(&cmd.args, str_array_get(file_paths, i), NULL);

	int ret = run_command(&cmd);
	child_process_def_release(&cmd);

	return ret;
}

int git_commit_index(const char *commit_message)
{
	return git_commit_index_with_options(commit_message, NULL);
}

int git_commit_index_with_options(const char *commit_message, ...)
{
	va_list varargs;
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	argv_array_push(&cmd.args, "commit", "-m", commit_message, NULL);

	va_start(varargs, commit_message);
	const char *arg;
	while ((arg = va_arg(varargs, char *)))
		argv_array_push(&cmd.args, arg, NULL);

	va_end(varargs);

	int ret = run_command(&cmd);
	child_process_def_release(&cmd);

	return ret;
}

int get_author_identity(struct strbuf *result)
{
	struct child_process_def cmd;
	struct strbuf cmd_out;
	int ret = 0;

	strbuf_init(&cmd_out);
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;

	const char * const config_keys[] = {
			"user.username",
			"user.email",
			"user.name",
			NULL
	};

	const char * const *config_key = config_keys;
	while (*config_key) {
		strbuf_clear(&cmd_out);
		str_array_clear((struct str_array *)&cmd.args);

		argv_array_push(&cmd.args, "config", "--get", *config_key, NULL);
		ret = capture_command(&cmd, &cmd_out);
		if (!ret) {
			strbuf_trim(&cmd_out);
			strbuf_attach_str(result, cmd_out.buff);

			strbuf_release(&cmd_out);
			child_process_def_release(&cmd);

			return 0;
		}

		config_key++;
	}

	child_process_def_release(&cmd);
	strbuf_release(&cmd_out);

	return 1;
}