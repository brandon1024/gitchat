#include <stdio.h>

#include "test-lib.h"
#include "test-suite.h"

static struct suite_test tests[] = {
		{ "str-array", str_array_test },
		{ "argv-array", argv_array_test },
		{ "strbuf", strbuf_test },
		{ "run-command", run_command_test },
		{ "fs-utils", fs_utils_test },
		{ "parse-options", parse_options_test },
		{ "config-data", config_data_test },
		{ "config-key", config_key_test },
		{ "config-defaults", config_defaults_test },
		{ "node-visitor", node_visitor_test },
		{ "parse-config", parse_config_test },
		{ "git-commit-parse", git_commit_parse_test },
		{ NULL, NULL }
};

int main(void)
{
	int verbose = 0, immediate = 0;

	char *env = getenv("GIT_CHAT_TEST_VERBOSE");
	if (env && strcmp(env, "") != 0 && strcmp(env, "0") != 0)
		verbose = 1;

	env = getenv("GIT_CHAT_TEST_IMMEDIATE");
	if (env && strcmp(env, "") != 0 && strcmp(env, "0") != 0)
		immediate = 1;

	return execute_suite(tests, verbose, immediate);
}
