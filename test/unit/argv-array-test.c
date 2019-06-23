#include "test-lib.h"
#include "argv-array.h"

TEST_DEFINE(argv_array_init_test)
{
	struct argv_array argv_a;
	argv_array_init(&argv_a);

	TEST_START() {
		assert_null(argv_a.arr.strings);
		assert_zero(argv_a.arr.len);
		assert_zero(argv_a.arr.alloc);
	}

	argv_array_release(&argv_a);
	TEST_END();
}

TEST_DEFINE(argv_array_release_test)
{
	struct argv_array argv_a;
	argv_array_init(&argv_a);
	argv_array_push(&argv_a, "str1", "str2", "str3", "str4", "str5", NULL);
	argv_array_release(&argv_a);

	TEST_START() {
		assert_null_msg(argv_a.arr.strings, "argv_array_release() should reinitialize the structure");
		assert_zero_msg(argv_a.arr.len, "argv_array_release() should reinitialize the structure");
		assert_zero_msg(argv_a.arr.alloc, "argv_array_release() should reinitialize the structure");
	}

	argv_array_release(&argv_a);
	TEST_END();
}

TEST_DEFINE(argv_array_push_test)
{
	struct argv_array argv_a;
	argv_array_init(&argv_a);

	TEST_START() {
		int ret = argv_array_push(&argv_a, NULL);
		assert_zero_msg(ret, "Pushing no elements to argv_array should return 0.");
		assert_zero_msg(argv_a.arr.alloc, "Pushing no elements to argv_array should not realloc.");
		assert_zero_msg(argv_a.arr.len, "Length of argv_array should remain zero after pushing no elements.");

		ret = argv_array_push(&argv_a, "str1", "str2", NULL);
		assert_eq_msg(2, ret, "Pushing two elements to argv_array should return 2, but was %d.", ret);
		assert_true_msg(argv_a.arr.alloc >= 2, "Pushing two elements to argv_array should alloc at least 2 elements.");
		assert_eq_msg(2, argv_a.arr.len, "Expected argv_array length of 2, but was %zu.", argv_a.arr.len);

		assert_string_eq("str1", argv_a.arr.strings[0]);
		assert_string_eq("str2", argv_a.arr.strings[1]);

		ret = argv_array_push(&argv_a, "str3", "str4", "str5", NULL);
		assert_eq_msg(3, ret, "Pushing three elements to argv_array should return 3, but was %d.", ret);

		ret = argv_array_push(&argv_a, "str6", "str7", "str8", NULL);
		assert_eq_msg(3, ret, "Pushing three elements to argv_array should return 3, but was %d.", ret);
		assert_true(argv_a.arr.alloc >= 8);
		assert_eq_msg(8, argv_a.arr.len, "Expected argv_array length of 8, but was %zu.", argv_a.arr.len);

		assert_string_eq("str3", argv_a.arr.strings[2]);
		assert_string_eq("str5", argv_a.arr.strings[4]);
	}

	argv_array_release(&argv_a);
	TEST_END();
}

TEST_DEFINE(argv_array_pop_test)
{
	struct argv_array argv_a;
	argv_array_init(&argv_a);

	char *popped_str = NULL;
	TEST_START() {
		popped_str = argv_array_pop(&argv_a);
		assert_null_msg(popped_str, "String popped from argv_array should be null if no elements exist");

		int len = argv_array_push(&argv_a, "str1", "str2", "str3", "str4", "str5", NULL);
		popped_str = argv_array_pop(&argv_a);
		assert_nonnull_msg(popped_str, "String popped from argv_array should not be null.");
		assert_string_eq("str5", popped_str);
		assert_eq_msg(len - 1, argv_a.arr.len, "Expected length of %d but was %zu.", argv_a.arr.len);
	}

	free(popped_str);
	argv_array_release(&argv_a);
	TEST_END();
}

TEST_DEFINE(argv_array_prepend_test)
{
	struct argv_array argv_a;
	argv_array_init(&argv_a);

	TEST_START() {
		argv_array_push(&argv_a, "str1", "str2", "str3", "str4", "str5", NULL);
		int ret = argv_array_prepend(&argv_a, "str0", NULL);
		assert_eq_msg(1, ret, "Prepending single string to argv_array should return 1, but got %d.", ret);
		assert_string_eq("str0", argv_a.arr.strings[0]);
	}

	argv_array_release(&argv_a);
	TEST_END();
}

TEST_DEFINE(argv_array_detach_test)
{
	struct argv_array argv_a;
	argv_array_init(&argv_a);
	char **strings = NULL;
	size_t len = 0;

	TEST_START() {
		int ret = argv_array_push(&argv_a, "str1", "str2", "str3", "str4", "str5", NULL);
		strings = argv_array_detach(&argv_a, &len);
		assert_eq_msg(ret, len, "Detaching string list from string should set len to %d, but got %d.", ret, len);
		assert_nonnull(strings);

		assert_null_msg(argv_a.arr.strings, "argv_array_detach() should reinitialize the structure");
		assert_zero_msg(argv_a.arr.len, "argv_array_detach() should reinitialize the structure");
		assert_zero_msg(argv_a.arr.alloc, "argv_array_detach() should reinitialize the structure");

		assert_string_eq("str1", strings[0]);
		assert_string_eq("str5", strings[4]);
	}

	if(strings) {
		for (size_t i = 0; i < len; i++)
			free(strings[i]);
	}

	argv_array_release(&argv_a);
	TEST_END();
}

TEST_DEFINE(argv_array_collapse_test)
{
	struct argv_array argv_a;
	argv_array_init(&argv_a);
	char *string = NULL;

	TEST_START() {
		string = argv_array_collapse(&argv_a);
		assert_null_msg(string, "Collapsing empty argv_array should return null.");

		argv_array_push(&argv_a, "str0", NULL);
		string = argv_array_pop(&argv_a);
		free(string);
		string = argv_array_collapse(&argv_a);
		assert_null_msg(string, "Collapsing empty argv_array should return null.");

		argv_array_push(&argv_a, "str0", NULL);
		string = argv_array_collapse(&argv_a);
		assert_nonnull(string);
		assert_eq_msg(1, argv_a.arr.len, "Expected argv_array length to remain 1, but was %zu.", argv_a.arr.len);
		assert_string_eq("str0", argv_a.arr.strings[0]);
		assert_string_eq("str0", string);

		free(string);

		argv_array_push(&argv_a, "str1", "str2", "str3", "str4", "str5", NULL);
		string = argv_array_collapse(&argv_a);
		assert_nonnull(string);
		assert_eq_msg(6, argv_a.arr.len, "Expected argv_array length to remain 6, but was %zu.", argv_a.arr.len);
		assert_string_eq("str0", argv_a.arr.strings[0]);
		assert_string_eq("str0 str1 str2 str3 str4 str5", string);
	}

	free(string);
	argv_array_release(&argv_a);
	TEST_END();
}

TEST_DEFINE(argv_array_collapse_delim_test)
{
	struct argv_array argv_a;
	argv_array_init(&argv_a);
	char *string = NULL;

	TEST_START() {
		string = argv_array_collapse_delim(&argv_a, "*");
		assert_null_msg(string, "Collapsing empty argv_array should return null.");

		argv_array_push(&argv_a, "str0", NULL);
		string = argv_array_collapse_delim(&argv_a, "*");
		assert_nonnull(string);
		assert_eq_msg(1, argv_a.arr.len, "Expected argv_array length to remain 1, but was %zu.", argv_a.arr.len);
		assert_string_eq("str0", argv_a.arr.strings[0]);
		assert_string_eq("str0", string);

		free(string);

		argv_array_push(&argv_a, "str1", "str2", "str3", "str4", "str5", NULL);
		string = argv_array_collapse_delim(&argv_a, "***");
		assert_nonnull(string);
		assert_eq_msg(6, argv_a.arr.len, "Expected argv_array length to remain 6, but was %zu.", argv_a.arr.len);
		assert_string_eq("str0", argv_a.arr.strings[0]);
		assert_string_eq(string, "str0***str1***str2***str3***str4***str5");
	}

	free(string);
	argv_array_release(&argv_a);
	TEST_END();
}

int argv_array_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "argv-array should initialize correctly", argv_array_init_test },
			{ "argv-array should release correctly", argv_array_release_test },
			{ "pushing to argv-array should correctly append strings", argv_array_push_test },
			{ "popping from argv-array should correctly remove strings", argv_array_pop_test },
			{ "prepending to argv-array should correctly prepend strings", argv_array_prepend_test },
			{ "detaching strings from argv-array should have correct", argv_array_detach_test },
			{ "collapsing argv-array into string should match expected", argv_array_collapse_test },
			{ "collapsing argv-array into delimited string should match expected", argv_array_collapse_delim_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
