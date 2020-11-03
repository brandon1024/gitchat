#include "test-lib.h"

extern const char *suite_name;
extern int test_suite(struct test_runner_instance *instance);

int main(void)
{
	int immediate = 0;

	char *env = getenv("GIT_CHAT_TEST_IMMEDIATE");
	if (env && strcmp(env, "") != 0 && strcmp(env, "0") != 0)
		immediate = 1;

	return execute_suite(test_suite, suite_name, immediate);
}
