#include "git/index.h"
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
