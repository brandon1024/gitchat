#include "test-lib.h"
#include "run-command.h"

TEST_DEFINE(run_command_from_dir_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.dir = "/bin";
	cmd.executable = "echo";
	cmd.no_in = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;
	str_array_push(&cmd.env, "PATH=", NULL);
	
	TEST_START() {
		int ret = run_command(&cmd);
		assert_eq(0, ret);
	}
	
	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_executable_path_to_file_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.executable = "/bin/echo";
	cmd.no_in = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;

	TEST_START() {
		int ret = run_command(&cmd);
		assert_eq(0, ret);
	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_executable_on_path_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	str_array_push(&cmd.env, "PATH=/bin:/usr/bin", NULL);
	cmd.executable = "env";
	cmd.no_in = 1;
	cmd.no_out = 1;
	cmd.no_err = 1;

	TEST_START() {
		int ret = run_command(&cmd);
		assert_eq(0, ret);
	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_with_args_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);

	TEST_START() {

	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_with_env_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	
	TEST_START() {

	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_git_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	
	TEST_START() {

	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_no_std_in_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	
	TEST_START() {

	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_no_std_out_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	
	TEST_START() {

	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_no_stderr_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	
	TEST_START() {

	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_stderr_to_stdout_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	
	TEST_START() {

	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_child_exit_status_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);

	TEST_START() {

	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(capture_command_test)
{
	char *string = "This\nString should be returned\tthrough stdout.";
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	struct strbuf buffer;
	strbuf_init(&buffer);
	cmd.executable = "echo";
	argv_array_push(&cmd.args, string, NULL);


	TEST_START() {
		int ret = capture_command(&cmd, &buffer);
		assert_eq(0, ret);
		assert_string_eq(string, buffer.buff);
	}

	strbuf_release(&buffer);
	child_process_def_release(&cmd);
	TEST_END();
}

int run_command_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "Executing a child process from a given directory should correctly chdir to that directory", run_command_from_dir_test },
			{ "Executing a child process by providing a full path to the executable should correctly find the executable to run", run_command_executable_path_to_file_test },
			{ "Executing a child process by providing an executable that exists on the path should correctly find the executable to run", run_command_executable_on_path_test },
			{ "Executing a child process with arguments should pass the arguments to the executable", run_command_with_args_test },
			{ "Executing a child process with a custom environment should correctly merge the environment with the parent process's environment", run_command_with_env_test },
			{ "Executing a git command should correctly invoke the git executable", run_command_git_test },
			{ "Executing a child process with no_in should not read from stdin", run_command_no_std_in_test },
			{ "Executing a child process with no_out should not write to stdout", run_command_no_std_out_test },
			{ "Executing a child process with no_err should not write to stderr", run_command_no_stderr_test },
			{ "Executing a child process that redirects stderr to stdout should do so", run_command_stderr_to_stdout_test },
			{ "run_command() should return the exit status code of the child process that was run", run_command_child_exit_status_test },
			{ "Capturing stdout from a child process should correctly build the process output to a string buffer", capture_command_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}