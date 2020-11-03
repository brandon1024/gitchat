#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "test-lib.h"

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

struct test_runner_instance {
	int verbose;
	int immediate;
	unsigned int executed;
	unsigned int passed;
	unsigned int failed;
};

static void print_test_heading(const char *suite_name)
{
	fprintf(stderr, "*** %s ***\n", suite_name);
}

static void print_test_suite_summary(unsigned int passed, unsigned int failed)
{
	fprintf(stderr, "\n\nTest Execution Summary:\n");
	fprintf(stderr, "Executed: %d\n", passed + failed);
	fprintf(stderr, "Passed: %d\n", passed);
	fprintf(stderr, (failed ? ANSI_COLOR_RED : ANSI_COLOR_GREEN));
	fprintf(stderr, "Failed: %d\n" ANSI_COLOR_RESET, failed);
}

static void print_test_summary(const char *suite_name, int ret)
{
	time_t rawtime;
	struct tm *timeinfo;

	fprintf(stderr, ret ? ANSI_COLOR_RED : ANSI_COLOR_GREEN);

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	fprintf(stderr, "[%.2d:%.2d:%.2d] ",
		   timeinfo->tm_hour,
		   timeinfo->tm_min,
		   timeinfo->tm_sec);

	fprintf(stderr, "%s ", suite_name);
	int len = 96 - (int)strlen(suite_name);
	for (int i = 0; len > 0 && i < len; i++)
		fprintf(stderr, ".");
	fprintf(stderr, " %s\n" ANSI_COLOR_RESET, ret ? "fail" : "ok");
}

int execute_suite(int (*fn)(struct test_runner_instance *), const char *suite_name, int immediate)
{
	struct test_runner_instance instance = { 0, 0, 0, 0, 0 };
	int ret;

	instance.immediate = immediate;

	print_test_heading(suite_name);

	ret = fn(&instance);
	if (!instance.verbose)
		print_test_summary(suite_name, ret);

	print_test_suite_summary(instance.passed, instance.failed);
	return ret;
}

int execute_tests(struct test_runner_instance *instance, struct unit_test *tests)
{
	int ret, failed = 0;
	struct unit_test *test = tests;

	while (test->test_name) {
		ret = test->fn();
		print_test_summary(test->test_name, ret);

		instance->executed++;
		if (ret)
			instance->failed++;
		else
			instance->passed++;

		failed |= ret;
		if (ret && instance->immediate)
			break;

		test++;
	}

	return failed;
}

void print_assertion_failure_message(const char *file_path, int line_number,
		const char *func_name, const char *fmt, ...)
{
	int err = errno;

	const char *filename = strrchr(file_path, '/');
	if (filename && *(filename + 1))
		filename++;
	else if (!filename)
		filename = file_path;

	fprintf(stderr, ANSI_COLOR_RED "Assertion failed: %s:%d in %s\n\t", filename, line_number, func_name);

	va_list varargs;
	va_start(varargs, fmt);
	vfprintf(stderr, fmt, varargs);
	va_end(varargs);

	fprintf(stderr, "\n");

	if (errno > 0)
		fprintf(stderr, "\nerror: %s\n\n" ANSI_COLOR_RESET, strerror(err));
}
