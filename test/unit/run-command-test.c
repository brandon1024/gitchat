#include <unistd.h>

#include "test-lib.h"
#include "run-command.h"
#include "utils.h"

TEST_DEFINE(child_process_def_set_stdin)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);

	TEST_START() {
		assert_eq_msg(STDIN_INHERITED, (cmd.std_fd_info & 0x00f), "std_fd_info should initialize with STDIN_INHERITED set");
		child_process_def_stdin(&cmd, STDIN_PROVISIONED);
		assert_eq_msg(STDIN_PROVISIONED, (cmd.std_fd_info & 0x00f), "child_process_def_stdin() did not correctly set STDIN_PROVISIONED");
		assert_eq(STDIN_PROVISIONED | STDOUT_INHERITED | STDERR_INHERITED, cmd.std_fd_info);
	}

	child_process_def_release(&cmd);

	TEST_END();
}

TEST_DEFINE(child_process_def_set_stdout)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);

	TEST_START() {
		assert_eq_msg(STDOUT_INHERITED, (cmd.std_fd_info & 0x0f0), "std_fd_info should initialize with STDOUT_INHERITED set");
		child_process_def_stdout(&cmd, STDOUT_PROVISIONED);
		assert_eq_msg(STDOUT_PROVISIONED, (cmd.std_fd_info & 0x0f0), "child_process_def_stdin() did not correctly set STDOUT_PROVISIONED");
		assert_eq(STDIN_INHERITED | STDOUT_PROVISIONED | STDERR_INHERITED, cmd.std_fd_info);
	}

	child_process_def_release(&cmd);

	TEST_END();
}

TEST_DEFINE(child_process_def_set_stderr)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);

	TEST_START() {
		assert_eq_msg(STDERR_INHERITED, (cmd.std_fd_info & 0xf00), "std_fd_info should initialize with STDERR_INHERITED set");
		child_process_def_stderr(&cmd, STDERR_PROVISIONED);
		assert_eq_msg(STDERR_PROVISIONED, (cmd.std_fd_info & 0xf00), "child_process_def_stdin() did not correctly set STDERR_PROVISIONED");
		assert_eq(STDIN_INHERITED | STDOUT_INHERITED | STDERR_PROVISIONED, cmd.std_fd_info);
	}

	child_process_def_release(&cmd);

	TEST_END();
}

TEST_DEFINE(run_command_from_dir_test)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	child_process_def_stdout(&cmd, STDOUT_NULL);
	cmd.dir = "/bin";
	cmd.executable = "echo";
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
	child_process_def_stdout(&cmd, STDOUT_NULL);
	cmd.executable = "/bin/echo";

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
	child_process_def_stdout(&cmd, STDOUT_NULL);
	child_process_def_stderr(&cmd, STDERR_NULL);
	cmd.executable = "test";

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
	child_process_def_stdout(&cmd, STDOUT_NULL);
	cmd.git_cmd = 1;
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

TEST_DEFINE(run_command_provisioned_in)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	child_process_def_stdin(&cmd, STDIN_PROVISIONED);
	child_process_def_stdout(&cmd, STDOUT_NULL);
	cmd.executable = "cat";

	TEST_START() {
		assert_zero_msg(pipe(cmd.in_fd), "failed to initialize pipe");

		int pid = start_command(&cmd);
		assert_eq_msg(pid, cmd.pid, "start_command() returned an unexpected pid");

		close(cmd.in_fd[0]);

		//write to the pipe, enough to fill it under most circumstances
		int write_failed = 0;
		const char buffer[] = "All work and no play makes Jack a dull boy";
		for (int i = 0; i < 4096; i++) {
			size_t expected = sizeof(buffer);
			if (xwrite(cmd.in_fd[1], buffer, expected) != (ssize_t)expected) {
				write_failed = 1;
				break;
			}
		}

		assert_false_msg(write_failed, "write() did not return expected value");

		close(cmd.in_fd[1]);

		int ret = finish_command(&cmd);
		assert_zero_msg(ret, "finish_command() returned with non-zero value");
	}

	child_process_def_release(&cmd);

	TEST_END();
}

TEST_DEFINE(run_command_provisioned_out)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	child_process_def_stdin(&cmd, STDIN_PROVISIONED);
	child_process_def_stdout(&cmd, STDOUT_PROVISIONED);
	cmd.executable = "cat";

	TEST_START() {
		assert_zero_msg(pipe(cmd.in_fd), "failed to initialize pipe");
		assert_zero_msg(pipe(cmd.out_fd), "failed to initialize pipe");

		int pid = start_command(&cmd);
		assert_eq_msg(pid, cmd.pid, "start_command() returned an unexpected pid");

		close(cmd.in_fd[0]);
		close(cmd.out_fd[1]);

		//write to the pipe, but read from it as well to avoid filling it
		const char buffer[] = "All work and no play makes Jack a dull boy";
		char capture_buffer[1024];

		int write_failed = 0, read_failed = 0;
		ssize_t total_bytes_read = 0;
		for (int i = 0; i < 4096; i++) {
			size_t expected = sizeof(buffer);
			if (xwrite(cmd.in_fd[1], buffer, expected) != (ssize_t)expected) {
				write_failed = 1;
				break;
			}

			ssize_t bytes_read = xread(cmd.out_fd[0], capture_buffer, 1024);
			if (bytes_read < 0) {
				read_failed = 1;
				break;
			}

			total_bytes_read += bytes_read;
		}

		assert_false_msg(write_failed, "write() did not return expected value");
		assert_false_msg(read_failed, "read() did not return expected value");

		assert_eq_msg(4096 * sizeof(buffer), total_bytes_read,
				"expected output length of %d but was %d", 4096 * sizeof(buffer),
				total_bytes_read);

		close(cmd.in_fd[1]);
		close(cmd.out_fd[0]);

		int ret = finish_command(&cmd);
		assert_zero_msg(ret, "finish_command() returned with non-zero value");
	}

	child_process_def_release(&cmd);

	TEST_END();
}

TEST_DEFINE(run_command_provisioned_err)
{
	struct child_process_def cmd;
	child_process_def_init(&cmd);
	child_process_def_stdin(&cmd, STDIN_PROVISIONED);
	child_process_def_stdout(&cmd, STDOUT_NULL);
	child_process_def_stderr(&cmd, STDERR_PROVISIONED);
	cmd.executable = "tee";
	argv_array_push(&cmd.args, "/dev/stderr", NULL);

	TEST_START() {
		assert_zero_msg(pipe(cmd.in_fd), "failed to initialize pipe");
		assert_zero_msg(pipe(cmd.err_fd), "failed to initialize pipe");

		int pid = start_command(&cmd);
		assert_eq_msg(pid, cmd.pid, "start_command() returned an unexpected pid");

		close(cmd.in_fd[0]);
		close(cmd.err_fd[1]);

		//write to the pipe, but read from it as well to avoid filling it
		const char buffer[] = "All work and no play makes Jack a dull boy";
		char capture_buffer[1024];

		int write_failed = 0, read_failed = 0;
		ssize_t total_bytes_read = 0;
		for (int i = 0; i < 4096; i++) {
			size_t expected = sizeof(buffer);
			if (xwrite(cmd.in_fd[1], buffer, expected) != (ssize_t)expected) {
				write_failed = 1;
				break;
			}

			ssize_t bytes_read = read(cmd.err_fd[0], capture_buffer, 1024);
			if (bytes_read < 0) {
				read_failed = 1;
				break;
			}

			total_bytes_read += bytes_read;
		}

		assert_false_msg(write_failed, "write() did not return expected value");
		assert_false_msg(read_failed, "read() did not return expected value");

		assert_eq_msg(4096 * sizeof(buffer), total_bytes_read,
				"expected output length of %d but was %d", 4096 * sizeof(buffer),
					  total_bytes_read);

		close(cmd.in_fd[1]);
		close(cmd.err_fd[0]);

		int ret = finish_command(&cmd);
		assert_zero_msg(ret, "finish_command() returned with non-zero value");
	}

	child_process_def_release(&cmd);

	TEST_END();
}

TEST_DEFINE(run_command_chain_processes)
{
	struct child_process_def child_a;
	struct child_process_def child_b;

	child_process_def_init(&child_a);
	child_a.executable = "echo";
	argv_array_push(&child_a.args, "hi", NULL);
	child_process_def_stdout(&child_a, STDOUT_PROVISIONED);

	child_process_def_init(&child_b);
	child_b.executable = "cat";
	child_process_def_stdin(&child_b, STDIN_PROVISIONED);
	child_process_def_stdout(&child_b, STDOUT_PROVISIONED);

	TEST_START() {
		int chain_pipe[2];
		assert_zero_msg(pipe(chain_pipe), "failed to initialize pipe");

		// stdout of child_a refers to stdin of child_b
		child_a.out_fd[0] = chain_pipe[0];
		child_a.out_fd[1] = chain_pipe[1];
		child_b.in_fd[0] = chain_pipe[0];
		child_b.in_fd[1] = chain_pipe[1];

		assert_zero_msg(pipe(child_b.out_fd), "failed to initialize pipe");

		start_command(&child_a);
		start_command(&child_b);
		close(child_b.out_fd[1]);

		int ret = finish_command(&child_a);
		assert_eq_msg(0, ret, "non-zero exit status %d from child process 'a'", ret);

		close(chain_pipe[1]);
		ret = finish_command(&child_b);
		assert_eq_msg(0, ret, "non-zero exit status %d from child process 'b'", ret);
		close(chain_pipe[0]);

		// read child_b output to verify data was passed between processes
		char capture_buffer[1024];
		ssize_t bytes_read = read(child_b.out_fd[0], capture_buffer, 1024);
		assert_true_msg(bytes_read == strlen("hi\n") , "unexpected number of characters read from child_b process");
		assert_zero_msg(strncmp(capture_buffer, "hi\n", 3), "expected string 'hi\\n' from process_b output");

		close(child_b.out_fd[0]);
	}

	child_process_def_release(&child_a);
	child_process_def_release(&child_b);

	TEST_END();
}

const char *suite_name = SUITE_NAME;
int test_suite(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "child_process_def_stdin() should only set stdin flags", child_process_def_set_stdin },
			{ "child_process_def_stdout() should only set stdout flags", child_process_def_set_stdout },
			{ "child_process_def_stderr() should only set stderr flags", child_process_def_set_stderr },
			{ "Executing a child process from a given directory should correctly chdir to that directory", run_command_from_dir_test },
			{ "Executing a child process by providing a full path to the executable should correctly find the executable to run", run_command_executable_path_to_file_test },
			{ "Executing a child process by providing an executable that exists on the path should correctly find the executable to run", run_command_executable_on_path_test },
			{ "Executing a child process with a custom environment should correctly merge the environment with the parent process's environment", run_command_with_env_test },
			{ "Executing a git command should correctly invoke the git executable", run_command_git_test },
			{ "run_command() should return the exit status code of the child process that was run", run_command_child_exit_status_test },
			{ "Capturing stdout from a child process should correctly build the process output to a string buffer", capture_command_test },
			{ "Executing a child process with a provisioned stdin fd should correctly use that fd as stdin", run_command_provisioned_in },
			{ "Executing a child process with a provisioned stdout fd should correctly use that fd as stdout", run_command_provisioned_out },
			{ "Executing a child process with a provisioned stderr fd should correctly use that fd as stderr", run_command_provisioned_err },
			{ "Chaining child processes' streams should correctly pipe data between them", run_command_chain_processes },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
