#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "run-command.h"
#include "utils.h"

static int execute(struct child_process_def *cmd, int capture,
		struct strbuf *buffer);
static void merge_env(struct str_array *child_env);

void child_process_def_init(struct child_process_def *cmd)
{
	cmd->git_cmd = 0;
	cmd->discard_out = 0;

	argv_array_init(&cmd->args);
	str_array_init(&cmd->env);
}

void child_process_def_release(struct child_process_def *cmd)
{
	argv_array_release(&cmd->args);
	str_array_release(&cmd->env);
	child_process_def_init(cmd);
}

int run_command(struct child_process_def *cmd)
{
	return execute(cmd, 0, NULL);
}

int capture_command(struct child_process_def *cmd, struct strbuf *buffer)
{
	return execute(cmd, 1, buffer);
}

/**
 *
 * */
static int execute(struct child_process_def *cmd, int capture,
		struct strbuf *buffer)
{
	const size_t BUFF_LEN = 1024;
	char buff[BUFF_LEN];

	if (capture && !buffer)
		BUG("Command output capture enabled but buffer is NULL.");

	if (cmd->git_cmd)
		argv_array_prepend(&cmd->args, "git", NULL);
	else if (cmd->executable)
		argv_array_prepend(&cmd->args, cmd->executable, NULL);
	else
		BUG("Unexpected child_process_def with executable specified.");

	char *command = argv_array_collapse(&cmd->args);
	if (command == NULL)
		BUG("Unexpected child_process_def with no arguments.");

	LOG_TRACE("executing shell process '%s'", command);

	//todo env is not used right now
	merge_env(&cmd->env);

	FILE *fp = popen(command, "r");
	free(command);

	if (fp == NULL)
		FATAL("Unable to create pipe to shell process.");

	while (fgets(buff, BUFF_LEN, fp) != NULL) {
		if (cmd->discard_out || capture)
			strbuf_attach(buffer, buff, BUFF_LEN);
		else
			fprintf(stdout, "%s", buff);
	}

	int status = pclose(fp);
	if (status == -1)
		FATAL("Unable to close pipe to shell process.");

	return status;
}

static void env_variable_key(char *env_var, struct strbuf *buff)
{
	char *eq = strchr(env_var, '=');
	struct strbuf c_key;
	strbuf_init(&c_key);

	strbuf_attach(buff, env_var, eq ? eq - env_var : strlen(env_var));
}

/**
 * Merge the current process environment into the array of desired environment
 * variables for the child process.
 *
 * Variables in the desired child process that also exist in the current process
 * will take precedence.
 * */
static void merge_env(struct str_array *deltaenv)
{
	extern char **environ;
	struct str_array current_env;
	str_array_init(&current_env);
	while (*environ)
		str_array_push(&current_env, *(environ++), NULL);

	str_array_sort(&current_env);
	str_array_sort(deltaenv);

	size_t p = 0, c = 0;
	while (p < current_env.len && c < deltaenv->len) {
		struct strbuf p_key;
		strbuf_init(&p_key);
		env_variable_key(current_env.strings[p], &p_key);

		struct strbuf c_key;
		strbuf_init(&c_key);
		env_variable_key(deltaenv->strings[c], &c_key);

		/* If keys are equal, child variable will take precedence */
		int cmp = strcmp(c_key.buff, p_key.buff);
		if (cmp > 0) {
			str_array_insert(deltaenv, 0, current_env.strings[p]);
			c++, p++;
		} else if (!cmp)
			p++, c++;
		else
			c++;

		strbuf_release(&p_key);
		strbuf_release(&c_key);
	}

	/* Add any remaining variables from parent */
	while (p < current_env.len)
		str_array_push(deltaenv, current_env.strings[p++], NULL);

	str_array_release(&current_env);
}