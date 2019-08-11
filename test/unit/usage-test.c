#include "test-lib.h"
#include "usage.h"

static const struct option_description options[] = {
		/* 0 */
		OPT_BOOL('q', "quiet", "suppress summary after successful commit"),
		OPT_BOOL('v', "verbose", "show diff in commit message template"),
		OPT_STRING('F', "file", "file", "read message from file"),
		OPT_LONG_STRING("author", "author", "override author for commit"),
		OPT_LONG_STRING("date", "date", "override date for commit"),

		/* 5 */
		OPT_STRING('m', "message", "message", "commit message"),
		OPT_STRING('c', "reedit-message", "commit", "reuse and edit message from specified commit"),
		OPT_LONG_STRING("reuse-message", "commit", "reuse message from specified commit"),
		OPT_BOOL('s', "sign-off", "add Signed-off-by:"),
		OPT_BOOL('e', "edit", "force edit of commit"),

		/* 10 */
		OPT_SHORT_STRING('C', "default", "how to strip spaces and #comments from message"),
		OPT_LONG_BOOL("status", "include status in commit message template"),
		OPT_SHORT_BOOL('S', "GPG sign commit"),
		OPT_LONG_STRING("gpg-sign", "key-id", "GPG sign commit"),
		OPT_BOOL('a', "all", "commit all changed files"),

		/* 15 */
		OPT_BOOL('i', "include", "add specified files to index for commit"),
		OPT_LONG_BOOL("interactive", "interactively add files"),
		OPT_BOOL('p', "patch", "interactively add changes"),
		OPT_BOOL('o', "only", "commit only specified files"),
		OPT_BOOL('n', "no-verify", "bypass pre-commit and commit-msg hooks"),

		/* 20 */
		OPT_LONG_BOOL("dry-run", "show what would be committed"),
		OPT_LONG_BOOL("short", "show status concisely"),
		OPT_LONG_BOOL("branch", "show branch information"),
		OPT_LONG_BOOL("ahead-behind", "compute full ahead/behind values"),
		OPT_LONG_BOOL("porcelain", "machine-readable output"),

		/* 25 */
		OPT_LONG_BOOL("long", "show status in long format"),
		OPT_BOOL('z', "null", "terminate entries with NUL"),
		OPT_LONG_BOOL("amend", "amend previous commit"),
		OPT_LONG_BOOL("no-post-rewrite", "bypass post-rewrite hook"),
		OPT_SHORT_BOOL('u', "show all untracked files"),

		/* 30 */
		OPT_LONG_BOOL("untracked-files", "show untracked files"),
		OPT_SHORT_INT('N', "show n things"),
		OPT_LONG_INT("display", "show n things"),
		OPT_INT('I', "idempotent", "show n idempotent things"),
		OPT_CMD("add", "add something"),

		/* 35 */
		OPT_CMD("rename", "rename something"),
		OPT_CMD("remove", "remove something"),

		OPT_END()
};

TEST_DEFINE(argument_matches_option_short_bool_test)
{
	int ret = 0;

	TEST_START() {
		/* matches */
		ret = argument_matches_option("-q", options[0]);
		assert_nonzero(ret);
		ret = argument_matches_option("-S", options[12]);
		assert_nonzero(ret);
		ret = argument_matches_option("-u", options[29]);
		assert_nonzero(ret);

		/* does not match */
		ret = argument_matches_option("-q", options[12]);
		assert_zero(ret);
		ret = argument_matches_option("--u", options[29]);
		assert_zero(ret);
		ret = argument_matches_option("testarg", options[0]);
		assert_zero(ret);
		ret = argument_matches_option("test--arg", options[0]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_short_combined_bool_test)
{
	int ret = 0;

	TEST_START() {
		/* matches */
		ret = argument_matches_option("-vq", options[0]);
		assert_nonzero(ret);
		ret = argument_matches_option("-Sv", options[12]);
		assert_nonzero(ret);
		ret = argument_matches_option("-Suq", options[29]);
		assert_nonzero(ret);

		/* does not match */
		ret = argument_matches_option("-pon", options[0]);
		assert_zero(ret);
		ret = argument_matches_option("--v", options[1]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_short_int_test)
{
	int ret = 0;

	TEST_START() {
		/* matches */
		ret = argument_matches_option("-N", options[31]);
		assert_nonzero(ret);

		/* does not match */
		ret = argument_matches_option("-I", options[33]);
		assert_nonzero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_short_string_test)
{
	int ret = 0;

	TEST_START() {
		/* matches */
		ret = argument_matches_option("-C", options[10]);
		assert_nonzero(ret);

		/* does not match */
		ret = argument_matches_option("-c", options[10]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_long_bool_test)
{
	int ret = 0;

	TEST_START() {
		ret = argument_matches_option("--interactive", options[16]);
		assert_nonzero(ret);

		ret = argument_matches_option("--long", options[25]);
		assert_nonzero(ret);

		ret = argument_matches_option("--no-post-rewrite", options[28]);
		assert_nonzero(ret);

		ret = argument_matches_option("--interactive", options[25]);
		assert_zero(ret);
		ret = argument_matches_option("--i", options[25]);
		assert_zero(ret);
		ret = argument_matches_option("-l", options[25]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_long_int_test)
{
	int ret = 0;

	TEST_START() {
		ret = argument_matches_option("--display", options[32]);
		assert_nonzero(ret);

		ret = argument_matches_option("display", options[32]);
		assert_zero(ret);
		ret = argument_matches_option("-display", options[32]);
		assert_zero(ret);
		ret = argument_matches_option("--display", options[33]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_long_string_test)
{
	int ret = 0;

	TEST_START() {
		ret = argument_matches_option("--gpg-sign", options[13]);
		assert_nonzero(ret);

		ret = argument_matches_option("--reuse-message", options[7]);
		assert_nonzero(ret);

		ret = argument_matches_option("--date", options[4]);
		assert_nonzero(ret);

		ret = argument_matches_option("date", options[4]);
		assert_zero(ret);
		ret = argument_matches_option("-date", options[4]);
		assert_zero(ret);
		ret = argument_matches_option("--date", options[7]);
		assert_zero(ret);
		ret = argument_matches_option("--DATE", options[4]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_bool_test)
{
	int ret = 0;

	TEST_START() {
		ret = argument_matches_option("-q", options[0]);
		assert_nonzero(ret);
		ret = argument_matches_option("--quiet", options[0]);
		assert_nonzero(ret);

		ret = argument_matches_option("-s", options[8]);
		assert_nonzero(ret);
		ret = argument_matches_option("--sign-off", options[8]);
		assert_nonzero(ret);

		ret = argument_matches_option("-Sop", options[17]);
		assert_nonzero(ret);
		ret = argument_matches_option("-Sop", options[18]);
		assert_nonzero(ret);

		ret = argument_matches_option("--q", options[0]);
		assert_zero(ret);
		ret = argument_matches_option("quiet", options[0]);
		assert_zero(ret);
		ret = argument_matches_option("--QUIET", options[0]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_int_test)
{
	int ret = 0;

	TEST_START() {
		ret = argument_matches_option("-I", options[33]);
		assert_nonzero(ret);
		ret = argument_matches_option("--idempotent", options[33]);
		assert_nonzero(ret);

		ret = argument_matches_option("-i", options[33]);
		assert_zero(ret);
		ret = argument_matches_option("i", options[33]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_string_test)
{
	int ret = 0;

	TEST_START() {
		ret = argument_matches_option("-F", options[2]);
		assert_nonzero(ret);
		ret = argument_matches_option("--file", options[2]);
		assert_nonzero(ret);
		ret = argument_matches_option("-qvF", options[2]);
		assert_nonzero(ret);

		ret = argument_matches_option("-m", options[5]);
		assert_nonzero(ret);
		ret = argument_matches_option("--message", options[5]);
		assert_nonzero(ret);

		ret = argument_matches_option("--F", options[2]);
		assert_zero(ret);
		ret = argument_matches_option("-file", options[2]);
		assert_zero(ret);
		ret = argument_matches_option("-qFv", options[2]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(argument_matches_option_cmd_test)
{
	int ret = 0;

	TEST_START() {
		ret = argument_matches_option("add", options[34]);
		assert_nonzero(ret);

		ret = argument_matches_option("rename", options[35]);
		assert_nonzero(ret);

		ret = argument_matches_option("remove", options[36]);
		assert_nonzero(ret);

		ret = argument_matches_option("--add", options[34]);
		assert_zero(ret);
		ret = argument_matches_option("--rename", options[35]);
		assert_zero(ret);
		ret = argument_matches_option("--remove", options[36]);
		assert_zero(ret);
		ret = argument_matches_option("add--", options[34]);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(is_valid_argument_bool_test)
{
	int ret = 0;

	TEST_START() {
		ret = is_valid_argument("-q", options);
		assert_nonzero(ret);
		ret = is_valid_argument("--quiet", options);
		assert_nonzero(ret);
		ret = is_valid_argument("--message", options);
		assert_nonzero(ret);
		ret = is_valid_argument("--no-verify", options);
		assert_nonzero(ret);

		ret = is_valid_argument("-qvC", options);
		assert_nonzero(ret);
		ret = is_valid_argument("-pon", options);
		assert_nonzero(ret);

		ret = is_valid_argument("--pon", options);
		assert_zero(ret);
		ret = is_valid_argument("-pOn", options);
		assert_zero(ret);
		ret = is_valid_argument("-P", options);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(is_valid_argument_int_test)
{
	int ret = 0;

	TEST_START() {
		ret = is_valid_argument("-I", options);
		assert_nonzero(ret);
		ret = is_valid_argument("--idempotent", options);
		assert_nonzero(ret);
		ret = is_valid_argument("-N", options);
		assert_nonzero(ret);

		ret = is_valid_argument("-qvI", options);
		assert_nonzero(ret);
		ret = is_valid_argument("-qvN", options);
		assert_nonzero(ret);

		ret = is_valid_argument("-qIv", options);
		assert_zero(ret);
		ret = is_valid_argument("-qNv", options);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(is_valid_argument_string_test)
{
	int ret = 0;

	TEST_START() {
		ret = is_valid_argument("-C", options);
		assert_nonzero(ret);
		ret = is_valid_argument("--gpg-sign", options);
		assert_nonzero(ret);
		ret = is_valid_argument("-F", options);
		assert_nonzero(ret);

		ret = is_valid_argument("-qvF", options);
		assert_nonzero(ret);
		ret = is_valid_argument("-qvC", options);
		assert_nonzero(ret);

		ret = is_valid_argument("-qFv", options);
		assert_zero(ret);
		ret = is_valid_argument("-qCv", options);
		assert_zero(ret);
	}

	TEST_END();
}

TEST_DEFINE(is_valid_argument_command_test)
{
	int ret = 0;

	TEST_START() {
		ret = is_valid_argument("rename", options);
		assert_nonzero(ret);
		ret = is_valid_argument("remove", options);
		assert_nonzero(ret);
		ret = is_valid_argument("add", options);
		assert_nonzero(ret);

		ret = is_valid_argument("--rename", options);
		assert_zero(ret);
		ret = is_valid_argument("--add", options);
		assert_zero(ret);
	}

	TEST_END();
}

int usage_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "short boolean arg should match correctly", argument_matches_option_short_bool_test },
			{ "short combined boolean args should match correctly", argument_matches_option_short_combined_bool_test },
			{ "short int arg should match correctly", argument_matches_option_short_int_test },
			{ "short string arg should match correctly", argument_matches_option_short_string_test },
			{ "long boolean arg should match correctly", argument_matches_option_long_bool_test },
			{ "long int arg should match correctly", argument_matches_option_long_int_test },
			{ "long string arg should match correctly", argument_matches_option_long_string_test },
			{ "short and long boolean arg should match correctly", argument_matches_option_bool_test },
			{ "short and long int arg should match correctly", argument_matches_option_int_test },
			{ "short and long string arg should match correctly", argument_matches_option_string_test },
			{ "command arg should match correctly", argument_matches_option_cmd_test },
			{ "is_valid_argument should differentiate valid and invalid boolean args", is_valid_argument_bool_test },
			{ "is_valid_argument should differentiate valid and invalid int args", is_valid_argument_int_test },
			{ "is_valid_argument should differentiate valid and invalid string args", is_valid_argument_string_test },
			{ "is_valid_argument should differentiate valid and invalid command args", is_valid_argument_command_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
