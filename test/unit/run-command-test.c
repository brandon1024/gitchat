#include "test-lib.h"
#include "run-command.h"

TEST_DEFINE(run_command_from_dir_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.dir = "/bin";
	cmd.executable = "echo";
	cmd.no_out = 1;
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
	cmd.no_out = 1;

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
	cmd.executable = "test";
	cmd.no_out = 1;
	cmd.no_err = 1;
	str_array_push(&cmd.env, "PATH=/bin:/usr/bin", NULL);
	argv_array_push(&cmd.args, "0", NULL);

	TEST_START() {
		int ret = run_command(&cmd);
		assert_eq(0, ret);
	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_with_env_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.executable = "printenv";
	argv_array_push(&cmd.args, "MY_VAR", NULL);
	str_array_push(&cmd.env, "MY_VAR=Hello World", NULL);

	struct strbuf output_buf;
	strbuf_init(&output_buf);

	TEST_START() {
		int ret = capture_command(&cmd, &output_buf);
		assert_eq(0, ret);
		assert_string_eq("Hello World\n", output_buf.buff);
	}

	strbuf_release(&output_buf);
	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_git_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.git_cmd = 1;
	cmd.no_out = 1;
	argv_array_push(&cmd.args, "--version", NULL);
	
	TEST_START() {
		int ret = run_command(&cmd);
		assert_eq(0, ret);
	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(run_command_child_exit_status_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.executable = "test";
	argv_array_push(&cmd.args, "0", NULL);

	TEST_START() {
		int ret = run_command(&cmd);
		assert_eq(0, ret);

		char *arg = argv_array_pop(&cmd.args);
		free(arg);
		ret = run_command(&cmd);
		assert_eq(1, ret);
	}

	child_process_def_release(&cmd);
	TEST_END();
}

TEST_DEFINE(capture_command_test)
{
	struct strbuf expected_buf;
	strbuf_init(&expected_buf);
	struct strbuf result_buf;
	strbuf_init(&result_buf);

	strbuf_attach_str(&expected_buf, "This.String should be returned\tthrough stdout.");
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	cmd.executable = "echo";
	argv_array_push(&cmd.args, expected_buf.buff, NULL);

	TEST_START() {
		int ret = capture_command(&cmd, &result_buf);
		assert_eq(0, ret);

		strbuf_attach_chr(&expected_buf, '\n');
		assert_string_eq(expected_buf.buff, result_buf.buff);
	}

	strbuf_release(&expected_buf);
	strbuf_release(&result_buf);
	child_process_def_release(&cmd);
	TEST_END();
}

int run_command_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "Executing a child process from a given directory should correctly chdir to that directory", run_command_from_dir_test },
			{ "Executing a child process by providing a full path to the executable should correctly find the executable to run", run_command_executable_path_to_file_test },
			{ "Executing a child process by providing an executable that exists on the path should correctly find the executable to run", run_command_executable_on_path_test },
			{ "Executing a child process with a custom environment should correctly merge the environment with the parent process's environment", run_command_with_env_test },
			{ "Executing a git command should correctly invoke the git executable", run_command_git_test },
			{ "run_command() should return the exit status code of the child process that was run", run_command_child_exit_status_test },
			{ "Capturing stdout from a child process should correctly build the process output to a string buffer", capture_command_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
