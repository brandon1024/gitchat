#include "test-lib.h"

#include "str-array.h"
#include "parse-options.h"

static struct command_option options[] = {
		/* 0 */ OPT_GROUP("Group 0"),
		/* 1 */ OPT_BOOL('q', "quiet", "suppress summary after successful commit", NULL),
		/* 2 */ OPT_BOOL('v', "verbose", "show diff in commit message template", NULL),
		/* 3 */ OPT_STRING('F', "file", "file", "read message from file", NULL),
		/* 4 */ OPT_LONG_STRING("author", "author", "override author for commit", NULL),
		/* 5 */ OPT_LONG_STRING("date", "date", "override date for commit", NULL),
		/* 6 */ OPT_GROUP("Group 1"),
		/* 7 */ OPT_STRING('m', "message", "message", "commit message", NULL),
		/* 8 */ OPT_SHORT_STRING('c', "commit", "reuse and edit message from specified commit", NULL),
		/* 9 */ OPT_LONG_STRING("reuse-message", "commit", "reuse message from specified commit", NULL),
		/* 10 */ OPT_BOOL('s', "sign-off", "add Signed-off-by:", NULL),
		/* 11 */ OPT_BOOL('e', "edit", "force edit of commit", NULL),
		/* 12 */ OPT_GROUP("Group 2"),
		/* 13 */ OPT_SHORT_STRING('C', "default", "how to strip spaces and #comments from message", NULL),
		/* 14 */ OPT_LONG_BOOL("status", "include status in commit message template", NULL),
		/* 15 */ OPT_SHORT_BOOL('S', "GPG sign commit", NULL),
		/* 16 */ OPT_LONG_STRING("gpg-sign", "key-id", "GPG sign commit", NULL),
		/* 17 */ OPT_BOOL('a', "all", "commit all changed files", NULL),
		/* 18 */ OPT_GROUP("Group 3"),
		/* 19 */ OPT_BOOL('i', "include", "add specified files to index for commit", NULL),
		/* 20 */ OPT_LONG_BOOL("interactive", "interactively add files", NULL),
		/* 21 */ OPT_BOOL('p', "patch", "interactively add changes", NULL),
		/* 22 */ OPT_INT('o', "only", "commit only specified files", NULL),
		/* 23 */ OPT_BOOL('n', "no-verify", "bypass pre-commit and commit-msg hooks", NULL),
		/* 24 */ OPT_GROUP("Group 4"),
		/* 25 */ OPT_LONG_BOOL("dry-run", "show what would be committed", NULL),
		/* 26 */ OPT_LONG_BOOL("short", "show status concisely", NULL),
		/* 27 */ OPT_LONG_BOOL("branch", "show branch information", NULL),
		/* 28 */ OPT_LONG_BOOL("ahead-behind", "compute full ahead/behind values", NULL),
		/* 29 */ OPT_LONG_BOOL("porcelain", "machine-readable output", NULL),
		/* 30 */ OPT_GROUP("Group 5"),
		/* 31 */ OPT_LONG_BOOL("long", "show status in long format", NULL),
		/* 32 */ OPT_BOOL('z', "null", "terminate entries with NUL", NULL),
		/* 33 */ OPT_LONG_BOOL("amend", "amend previous commit", NULL),
		/* 34 */ OPT_LONG_BOOL("no-post-rewrite", "bypass post-rewrite hook", NULL),
		/* 35 */ OPT_SHORT_BOOL('u', "show all untracked files", NULL),
		/* 36 */ OPT_GROUP("Group 6"),
		/* 37 */ OPT_LONG_INT("untracked-files", "show untracked files", NULL),
		/* 38 */ OPT_SHORT_INT('N', "show n things", NULL),
		/* 39 */ OPT_LONG_INT("display", "show n things", NULL),
		/* 40 */ OPT_INT('I', "idempotent", "show n idempotent things", NULL),
		/* 41 */ OPT_CMD("add", "add something", NULL),
		/* 42 */ OPT_GROUP("Group 7"),
		/* 43 */ OPT_CMD("rename", "rename something", NULL),
		/* 44 */ OPT_CMD("remove", "remove something", NULL),
		/* 45 */ OPT_STRING_LIST('R', "recipients", "recipients", "specify one or more recipients", NULL),
		/* 46 */ OPT_SHORT_STRING_LIST('D', "strings", "specify one or more strings", NULL),
		/* 47 */ OPT_LONG_STRING_LIST("values", "values", "specify one or more values", NULL),
		/* 48 */ OPT_SHORT_INT('A', "show n things", NULL),
		/* 49 */ OPT_SHORT_INT('B', "show n things", NULL),
		/* 50 */ OPT_END()
};

TEST_DEFINE(parse_options_short_bool_test)
{
	int bool_val_1 = 0;
	int bool_val_2 = 0;
	options[15].arg_value = &bool_val_1;
	options[35].arg_value = &bool_val_2;

	TEST_START() {
		const int argc = 1;
		char *argv[] = {"-S"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(0, new_argc);

		assert_true(bool_val_1);
		assert_false(bool_val_2);
	}

	options[15].arg_value = NULL;
	options[35].arg_value = NULL;
	TEST_END();
}

TEST_DEFINE(parse_options_short_int_test)
{
	int bool_val_1 = 0;
	int int_val_1 = 0;
	int int_val_2 = 0;
	int int_val_3 = 0;
	int int_val_4 = 0;
	options[1].arg_value = &bool_val_1;
	options[38].arg_value = &int_val_1;
	options[48].arg_value = &int_val_2;
	options[49].arg_value = &int_val_3;
	options[22].arg_value = &int_val_4;

	TEST_START() {
		int argc = 7;
		char *argv[] = {"-qN", "+12", "-A", "-13", "-o15", "-B", "12a"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(2, new_argc);

		assert_true(bool_val_1);
		assert_eq(12, int_val_1);
		assert_eq(-13, int_val_2);
		assert_eq(0, int_val_3);
		assert_eq(15, int_val_4);
		assert_string_eq("-B", argv[0]);
		assert_string_eq("12a", argv[1]);

		argc = 6;
		char *argv1[] = {"-N", "0x12", "-A", "0x13", "-I", "-a"};

		new_argc = parse_options(argc, argv1, options, 0, 1);
		assert_eq(2, new_argc);

		assert_eq(0x12, int_val_1);
		assert_eq(0x13, int_val_2);
	assert_string_eq("-I", argv1[0]);
	assert_string_eq("-a", argv1[1]);
	}

	// test teardown
	options[1].arg_value = NULL;
	options[38].arg_value = NULL;
	options[48].arg_value = NULL;
	options[49].arg_value = NULL;

	TEST_END();
}

TEST_DEFINE(parse_options_short_string_test)
{
	char *str_1 = NULL;
	char *str_2 = NULL;

	options[8].arg_value = &str_1;
	options[13].arg_value = &str_2;

	TEST_START() {
		int argc = 5;
		char *argv[] = {"-c", "my string", "-C", "my second string", "-c"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(1, new_argc);

		assert_string_eq("my string", str_1);
		assert_string_eq("my second string", str_2);
		assert_string_eq("-c", argv[0]);
	}

	options[8].arg_value = NULL;
	options[13].arg_value = NULL;

	TEST_END();
}

TEST_DEFINE(parse_options_short_string_list_test)
{
	struct str_array strings;
	str_array_init(&strings);

	options[46].arg_value = &strings;

	TEST_START() {
		int argc = 5;
		char *argv[] = {"-D", "my string", "-D", "my second string", "-D"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(1, new_argc);

		assert_eq(2, strings.len);
		assert_string_eq("my string", str_array_get(&strings, 0));
		assert_string_eq("my second string", str_array_get(&strings, 1));
		assert_string_eq("-D", argv[0]);
	}

	options[46].arg_value = NULL;
	str_array_release(&strings);

	TEST_END();
}

TEST_DEFINE(parse_options_long_bool_test)
{
	int bool_val_1 = 0;
	int bool_val_2 = 0;
	int bool_val_3 = 0;
	options[14].arg_value = &bool_val_1;
	options[20].arg_value = &bool_val_2;
	options[26].arg_value = &bool_val_3;

	TEST_START() {
		const int argc = 3;
		char *argv[] = {"--status", "--interactive", "--unknown-op"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(1, new_argc);

		assert_true(bool_val_1);
		assert_true(bool_val_2);
		assert_false(bool_val_3);
		assert_string_eq("--unknown-op", argv[0]);
	}

	options[14].arg_value = NULL;
	options[20].arg_value = NULL;
	options[26].arg_value = NULL;
	TEST_END();
}

TEST_DEFINE(parse_options_long_int_test)
{
	int int_val_1 = 0;
	int int_val_2 = 0;
	options[37].arg_value = &int_val_1;
	options[39].arg_value = &int_val_2;

	TEST_START() {
		const int argc = 3;
		char *argv[] = {"--untracked-files", "26", "--display=-9"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(0, new_argc);

		assert_eq(int_val_1, 26);
		assert_eq(int_val_2, -9);
	}

	options[37].arg_value = NULL;
	options[39].arg_value = NULL;
	TEST_END();
}

TEST_DEFINE(parse_options_long_string_test)
{
	char *string_val_1 = NULL;
	char *string_val_2 = NULL;
	char *string_val_3 = NULL;
	options[4].arg_value = &string_val_1;
	options[5].arg_value = &string_val_2;
	options[16].arg_value = &string_val_3;

	TEST_START() {
		const int argc = 6;
		char *argv[] = {"--author", "string1", "--date=string2", "--gpg-sign", "string3", "--reuse-message"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(1, new_argc);

		assert_string_eq("string1", string_val_1);
		assert_string_eq("string2", string_val_2);
		assert_string_eq("string3", string_val_3);
		assert_string_eq("--reuse-message", argv[0]);
	}

	options[4].arg_value = NULL;
	options[5].arg_value = NULL;
	options[9].arg_value = NULL;
	TEST_END();
}

TEST_DEFINE(parse_options_long_string_list_test)
{
	struct str_array strings;
	str_array_init(&strings);

	options[47].arg_value = &strings;

	TEST_START() {
		int argc = 5;
		char *argv[] = {"--values", "my string", "--values=string2", "--values", "another string"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_zero(new_argc);

		assert_eq(3, strings.len);
		assert_string_eq("my string", str_array_get(&strings, 0));
		assert_string_eq("string2", str_array_get(&strings, 1));
		assert_string_eq("another string", str_array_get(&strings, 2));
	}

	options[47].arg_value = NULL;
	str_array_release(&strings);

	TEST_END();
}

TEST_DEFINE(parse_options_bool_test)
{
	int bool_val_1 = 0;
	int bool_val_2 = 0;
	int bool_val_3 = 0;
	int bool_val_4 = 0;
	options[1].arg_value = &bool_val_1;
	options[2].arg_value = &bool_val_2;
	options[10].arg_value = &bool_val_3;
	options[23].arg_value = &bool_val_4;

	TEST_START() {
		const int argc = 3;
		char *argv[] = {"--quiet", "-vn", "--sign-off"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_zero(new_argc);

		assert_true(bool_val_1);
		assert_true(bool_val_2);
		assert_true(bool_val_3);
		assert_true(bool_val_4);
	}

	options[1].arg_value = NULL;
	options[2].arg_value = NULL;
	options[10].arg_value = NULL;
	options[23].arg_value = NULL;
	TEST_END();
}

TEST_DEFINE(parse_options_int_test)
{
	int bool_val_1 = 0;
	int int_val_1 = 0;
	int int_val_2 = 0;
	options[1].arg_value = &bool_val_1;
	options[22].arg_value = &int_val_1;
	options[40].arg_value = &int_val_2;

	TEST_START() {
		const int argc = 4;
		char *argv[] = {"--only", "-160", "-qI", "0xa1"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_zero(new_argc);

		assert_eq(-160, int_val_1);
		assert_true(bool_val_1);
		assert_eq(0xa1, int_val_2);
	}

	options[1].arg_value = NULL;
	options[22].arg_value = NULL;
	options[40].arg_value = NULL;
	TEST_END();
}

TEST_DEFINE(parse_options_string_test)
{
	char *str_1 = NULL;
	char *str_2 = NULL;

	int bool_val_1 = 0;
	options[1].arg_value = &bool_val_1;
	options[3].arg_value = &str_1;
	options[7].arg_value = &str_2;

	TEST_START() {
		const int argc = 4;
		char *argv[] = {"--file", "my file", "-qm", "0xa1"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_zero(new_argc);

		assert_true(bool_val_1);
		assert_string_eq("my file", str_1);
		assert_string_eq("0xa1", str_2);
	}

	options[1].arg_value = NULL;
	options[3].arg_value = NULL;
	options[7].arg_value = NULL;

	TEST_END();
}

TEST_DEFINE(parse_options_string_list_test)
{
	struct str_array strings;
	str_array_init(&strings);

	options[45].arg_value = &strings;

	TEST_START() {
		int argc = 5;
		char *argv[] = {"--recipients", "my string", "--recipients=string2", "-R", "another string"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_zero(new_argc);

		assert_eq(3, strings.len);
		assert_string_eq("my string", str_array_get(&strings, 0));
		assert_string_eq("string2", str_array_get(&strings, 1));
		assert_string_eq("another string", str_array_get(&strings, 2));
	}

	options[45].arg_value = NULL;
	str_array_release(&strings);

	TEST_END();
}

TEST_DEFINE(parse_options_cmd_test)
{
	int bool_val_1 = 0;
	int cmd_val_1 = 0;
	options[1].arg_value = &bool_val_1;
	options[43].arg_value = &cmd_val_1;

	TEST_START() {
		int argc = 5;
		char *argv[] = {"-q", "add", "--recipients=string2", "-R", "another string"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(4, new_argc);

		assert_true(bool_val_1);
		assert_string_eq("add", argv[0]);
		assert_string_eq("--recipients=string2", argv[1]);
		assert_string_eq("-R", argv[2]);
		assert_string_eq("another string", argv[3]);

		char *argv1[] = {"-q", "rename", "--recipients=string2", "-R", "another string"};
		new_argc = parse_options(argc, argv1, options, 0, 1);
		assert_eq(4, new_argc);

		assert_true(cmd_val_1);
		assert_string_eq("rename", argv1[0]);
		assert_string_eq("--recipients=string2", argv1[1]);
		assert_string_eq("-R", argv1[2]);
		assert_string_eq("another string", argv1[3]);
	}

	options[1].arg_value = NULL;

	TEST_END();
}

TEST_DEFINE(parse_options_skip_first_arg_test)
{
	int bool_val_1 = 0;
	int bool_val_2 = 0;
	options[15].arg_value = &bool_val_1;
	options[35].arg_value = &bool_val_2;

	TEST_START() {
		const int argc = 2;
		char *argv[] = {"-S", "-u"};

		int new_argc = parse_options(argc, argv, options, 1, 1);
		assert_eq(0, new_argc);

		assert_false(bool_val_1);
		assert_true(bool_val_2);
	}

	options[15].arg_value = NULL;
	options[35].arg_value = NULL;
	TEST_END();
}

TEST_DEFINE(parse_options_unknown_arg_test)
{
	char *str_1 = NULL;
	char *str_2 = NULL;

	int bool_val_1 = 0;
	options[1].arg_value = &bool_val_1;
	options[3].arg_value = &str_1;
	options[7].arg_value = &str_2;

	TEST_START() {
		const int argc = 4;
		char *argv[] = {"--file", "my file", "-q4m", "0xa1"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(2, new_argc);

		assert_true(bool_val_1);
		assert_string_eq("my file", str_1);
	}

	options[1].arg_value = NULL;
	options[3].arg_value = NULL;
	options[7].arg_value = NULL;

	TEST_END();
}

TEST_DEFINE(parse_options_combined_short_arg_test)
{
	int bool_val_1 = 0, bool_val_2 = 0, bool_val_3 = 0, bool_val_4 = 0;
	char *string = NULL;

	options[1].arg_value = &bool_val_1;
	options[2].arg_value = &bool_val_2;
	options[10].arg_value = &bool_val_3;
	options[11].arg_value = &bool_val_4;
	options[13].arg_value = &string;

	TEST_START() {
		const int argc = 2;
		char *argv[] = {"-qvsC", "test string"};

		int new_argc = parse_options(argc, argv, options, 0, 1);
		assert_eq(0, new_argc);

		assert_true(bool_val_1);
		assert_true(bool_val_2);
		assert_true(bool_val_3);
		assert_false(bool_val_4);

		assert_string_eq("test string", string);
	}

	options[1].arg_value = NULL;
	options[2].arg_value = NULL;
	options[10].arg_value = NULL;
	options[11].arg_value = NULL;
	options[13].arg_value = NULL;

	TEST_END();
}

const char *suite_name = SUITE_NAME;
int test_suite(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "short boolean option directive should parse argument correctly", parse_options_short_bool_test },
			{ "short integer option directive should parse argument correctly", parse_options_short_int_test },
			{ "short string option directive should parse argument correctly", parse_options_short_string_test },
			{ "short string list option directive should parse argument correctly", parse_options_short_string_list_test },
			{ "long boolean option directive should parse argument correctly", parse_options_long_bool_test },
			{ "long integer option directive should parse argument correctly", parse_options_long_int_test },
			{ "long string option directive should parse argument correctly", parse_options_long_string_test },
			{ "long string list option directive should parse argument correctly", parse_options_long_string_list_test },
			{ "mixed length boolean option directive should parse argument correctly", parse_options_bool_test },
			{ "mixed length integer option directive should parse argument correctly", parse_options_int_test },
			{ "mixed length string option directive should parse argument correctly", parse_options_string_test },
			{ "mixed length string list option directive should parse argument correctly", parse_options_string_list_test },
			{ "command option directive should stop parsing arguments", parse_options_cmd_test },
			{ "parse_options with skip_first enabled should ignore first argument in arg vector", parse_options_skip_first_arg_test },
			{ "parse_options with stop_on_unknown enabled should stop if unknown arg found in arg vector", parse_options_unknown_arg_test },
			{ "combined short options should parse correctly", parse_options_combined_short_arg_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
