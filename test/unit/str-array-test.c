#include "test-lib.h"
#include "str-array.h"

TEST_DEFINE(str_array_init_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		assert_null(str_a.strings);
		assert_zero(str_a.len);
		assert_zero(str_a.alloc);
	}

	TEST_END();
}

TEST_DEFINE(str_array_grow_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		size_t size = 10;
		str_array_grow(&str_a, size);
		assert_eq(size, str_a.alloc);

		/* resize str_array */
		size = 15;
		str_array_grow(&str_a, size);
		assert_eq(size, str_a.alloc);

		/* attempt to resize str_array smaller */
		size = 10;
		str_array_grow(&str_a, size);
		assert_eq(size, str_a.alloc);

		/* attempt to resize str_array smaller than len */
		str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);
		size = 6;
		str_array_grow(&str_a, size);
		assert_eq(size, str_a.alloc);

		str_array_grow(&str_a, 5);
		assert_eq(size, str_a.alloc);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_release_test)
{
	struct str_array str_a;
	str_array_init(&str_a);
	str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);
	str_array_release(&str_a);

	TEST_START() {
		assert_null(str_a.strings);
		assert_zero(str_a.len);
		assert_zero(str_a.alloc);
	}

	TEST_END();
}

TEST_DEFINE(str_array_get_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);
		str_array_push(&str_a, "str6", "str7", NULL);

		char *str = str_array_get(&str_a, 0);
		assert_string_eq("str1", str);

		str = str_array_get(&str_a, 4);
		assert_string_eq("str5", str);

		str = str_array_get(&str_a, 5);
		assert_string_eq("str6", str);

		str = str_array_get(&str_a, 6);
		assert_string_eq("str7", str);

		str = str_array_get(&str_a, 7);
		assert_null(str);
	}

	str_array_release(&str_a);
	TEST_END();
}

int str_array_set_test()
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);
		str_array_push(&str_a, "str6", "str7", NULL);

		char *str = str_array_get(&str_a, 2);
		assert_string_eq("str3", str);

		int ret = str_array_set(&str_a, "my string", 2);
		assert_zero(ret);

		str = str_array_get(&str_a, 2);
		assert_string_eq("my string", str);

		ret = str_array_set(&str_a, "my string", 7);
		assert_nonzero(ret);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_set_nodup_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);
		str_array_push(&str_a, "str6", "str7", NULL);

		char *new_str = strdup("my string");
		assert_nonnull(new_str);

		int ret = str_array_set_nodup(&str_a, new_str, 2);
		assert_zero(ret);

		char *str = str_array_get(&str_a, 2);
		assert_eq(new_str, str);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_push_test)
{
	struct str_array str_a;
	str_array_init(&str_a);
	int ret = 0;

	TEST_START() {
		ret = str_array_push(&str_a, NULL);
		assert_zero(ret);
		assert_zero(str_a.alloc);
		assert_zero(str_a.len);

		ret = str_array_push(&str_a, "str1", "str2", NULL);
		assert_eq(ret, 2);
		assert_true(str_a.alloc >= 2);
		assert_eq(str_a.len, 2);

		assert_string_eq("str1", str_a.strings[0]);
		assert_string_eq("str2", str_a.strings[1]);

		ret = str_array_push(&str_a, "str3", "str4", "str5", NULL);
		assert_eq(3, ret);

		ret = str_array_push(&str_a, "str6", "str7", "str8", NULL);
		assert_eq(3, ret);
		assert_true(str_a.alloc >= 8);
		assert_eq(8, str_a.len);

		assert_string_eq("str3", str_a.strings[2]);
		assert_string_eq("str5", str_a.strings[4]);
	}

	str_array_release(&str_a);
	TEST_END();
}

int str_array_vpush_test_helper(size_t args, ...)
{
	struct str_array str_a;
	str_array_init(&str_a);
	int ret = 0;

	TEST_START() {
		va_list ap;
		va_start(ap, args);
		ret = str_array_vpush(&str_a, ap);
		va_end(ap);

		assert_eq(args, ret);
		assert_true(str_a.alloc > 8);
		assert_eq(8, str_a.len);
		assert_string_eq("str3", str_a.strings[2]);
		assert_string_eq("str5", str_a.strings[4]);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_vpush_test)
{
	return str_array_vpush_test_helper(8, "str1", "str2", "str3", "str4", "str5", "str6", "str7", "str8", NULL);
}

TEST_DEFINE(str_array_insert_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		size_t pos = str_array_insert(&str_a, 0, "str1");
		assert_zero(pos);
		assert_string_eq("str1", str_a.strings[0]);

		pos = str_array_insert(&str_a, 0, "str-1");
		assert_zero(pos);
		assert_string_eq("str-1", str_a.strings[0]);

		pos = str_array_insert(&str_a, 1, "str0");
		assert_eq(1, pos);
		assert_string_eq("str0", str_a.strings[1]);

		pos = str_array_insert(&str_a, 10, "str10");
		assert_eq(3, pos);
		assert_string_eq("str10", str_a.strings[3]);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_insert_nodup_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		char *str = strdup("my string");
		assert_nonnull(str);

		size_t pos = str_array_insert(&str_a, 0, "str1");
		assert_zero(pos);
		assert_string_eq("str1", str_a.strings[0]);

		pos = str_array_insert_nodup(&str_a, 0, str);
		assert_zero(pos);
		assert_eq(str, str_a.strings[0]);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_sort_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		str_array_sort(&str_a);

		str_array_push(&str_a, "astr1", "dstr3", "nstr4", "bstr2", "fstr5", NULL);
		str_array_sort(&str_a);
		assert_string_eq("astr1", str_a.strings[0]);
		assert_string_eq("bstr2", str_a.strings[1]);
		assert_string_eq("dstr3", str_a.strings[2]);
		assert_string_eq("fstr5", str_a.strings[3]);
		assert_string_eq("nstr4", str_a.strings[4]);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_remove_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		int ret = str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);
		char *removed_str = str_array_remove(&str_a, 0);
		assert_string_eq("str1", removed_str);
		assert_string_eq("str2", str_a.strings[0]);
		assert_eq((ret - 1), str_a.len);

		free(removed_str);

		removed_str = str_array_remove(&str_a, 1);
		assert_string_eq("str3", removed_str);
		assert_string_eq("str4", str_a.strings[1]);
		assert_eq((ret - 2), str_a.len);

		free(removed_str);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_detach_test)
{
	struct str_array str_a;
	str_array_init(&str_a);
	char **strings = NULL;
	size_t len = 0;

	TEST_START() {
		int ret = str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);

		strings = str_array_detach(&str_a, &len);
		assert_eq(5, len);
		assert_eq(len, ret);
		assert_nonnull(strings);

		assert_null(str_a.strings);
		assert_zero(str_a.len);
		assert_zero(str_a.alloc);

		assert_string_eq("str1", strings[0]);
		assert_string_eq("str5", strings[4]);
	}

	if (strings) {
		for (size_t i = 0; i < len; i++)
			free(strings[i]);

		free(strings);
	}

	TEST_END();
}

int str_array_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "str-array should initialize correctly", str_array_init_test },
			{ "str-array should grow in size upon request", str_array_grow_test },
			{ "str-array should release correctly", str_array_release_test },
			{ "accessing element from str-array should return correct element", str_array_get_test },
			{ "setting element in str-array should set correct element", str_array_set_test },
			{ "setting element in str-array should set without duplication", str_array_set_nodup_test },
			{ "pushing elements to str-array should append to the end", str_array_push_test },
			{ "pushing elements to str-array using variadic variant should append to the end", str_array_vpush_test },
			{ "inserting element in str-array should shift elements correctly", str_array_insert_test },
			{ "inserting element in str-array should insert without duplication", str_array_insert_nodup_test },
			{ "sorting element in str-array should sort by strcmp() order", str_array_sort_test },
			{ "removing element from str-array should shift elements correctly", str_array_remove_test },
			{ "detaching element from str-array should correctly detach", str_array_detach_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
