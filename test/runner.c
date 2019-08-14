#include <stdio.h>

#include "test-lib.h"
#include "test-suite.h"

static struct suite_test tests[] = {
		{ "str-array", str_array_test },
		{ "argv-array", argv_array_test },
		{ "strbuf", strbuf_test },
		{ "parse-config", parse_config_test },
		{ "usage", usage_test },
		{ "run-command", run_command_test },
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
