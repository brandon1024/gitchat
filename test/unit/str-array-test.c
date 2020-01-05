#include "test-lib.h"
#include "str-array.h"

TEST_DEFINE(str_array_init_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		assert_null(str_a.entries);
		assert_zero(str_a.len);
		assert_zero(str_a.alloc);
		assert_zero(str_a.free_data);
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
	str_a.free_data = 0;
	str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);

	void *dummy_data = NULL;

	TEST_START() {
		dummy_data = malloc(sizeof(struct str_array));
		assert_nonnull_msg(dummy_data, "failed to allocate memory");

		str_array_release(&str_a);

		assert_null(str_a.entries);
		assert_zero(str_a.len);
		assert_zero(str_a.alloc);
		assert_zero(str_a.free_data);
	}

	free(dummy_data);
	str_array_release(&str_a);

	TEST_END();
}

TEST_DEFINE(str_array_release_with_data_test)
{
	struct str_array str_a;
	str_array_init(&str_a);
	str_a.free_data = 1;

	TEST_START() {
		void *dummy_data = malloc(sizeof(struct str_array));
		assert_nonnull_msg(dummy_data, "failed to allocate memory");

		struct str_array_entry *entry1 = str_array_insert(&str_a, "test string1", 0);
		entry1->data = dummy_data;
		struct str_array_entry *entry2 = str_array_insert(&str_a, "test string2", 0);
		entry2->data = NULL;

		str_array_release(&str_a);

		assert_null(str_a.entries);
		assert_zero(str_a.len);
		assert_zero(str_a.alloc);
		assert_zero(str_a.free_data);
	}

	str_array_release(&str_a);

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

TEST_DEFINE(str_array_set_test)
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

TEST_DEFINE(str_array_set_with_data_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	void *dummy_data = NULL;

	TEST_START() {
		dummy_data = malloc(sizeof(struct str_array));
		assert_nonnull_msg(dummy_data, "failed to allocate memory");

		str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);
		str_array_push(&str_a, "str6", "str7", NULL);

		struct str_array_entry *entry = str_array_get_entry(&str_a, 2);
		assert_string_eq("str3", entry->string);
		assert_null(entry->data);

		entry->data = dummy_data;

		int ret = str_array_set(&str_a, "my string", 2);
		assert_zero(ret);

		entry = str_array_get_entry(&str_a, 2);
		assert_string_eq("my string", entry->string);
		assert_null(entry->data);

		ret = str_array_set(&str_a, "my string", 7);
		assert_nonzero(ret);
	}

	free(dummy_data);

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

		assert_string_eq("str1", str_array_get(&str_a, 0));
		assert_string_eq("str2", str_array_get(&str_a, 1));
		assert_null(str_array_get_entry(&str_a, 0)->data);
		assert_null(str_array_get_entry(&str_a, 1)->data);

		ret = str_array_push(&str_a, "str3", "str4", "str5", NULL);
		assert_eq(3, ret);

		ret = str_array_push(&str_a, "str6", "str7", "str8", NULL);
		assert_eq(3, ret);
		assert_true(str_a.alloc >= 8);
		assert_eq(8, str_a.len);

		assert_string_eq("str3", str_array_get(&str_a, 2));
		assert_string_eq("str5", str_array_get(&str_a, 4));

		struct str_array_entry last_entry = str_a.entries[str_a.len];
		assert_null(last_entry.string);
		assert_null(last_entry.data);
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
		assert_string_eq("str3", str_array_get(&str_a, 2));
		assert_string_eq("str5", str_array_get(&str_a, 4));
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
	str_a.free_data = 1;

	TEST_START() {
		char *new_str = strdup("my string");
		assert_nonnull(new_str);

		struct str_array_entry *entry = str_array_insert(&str_a, "str1", 0);
		entry->data = new_str;
		assert_string_eq("str1", str_array_get(&str_a, 0));
		assert_string_eq("my string", str_array_get_entry(&str_a, 0)->data);

		new_str = strdup("my string 1");
		assert_nonnull(new_str);

		entry = str_array_insert(&str_a, "str-1", 0);
		entry->data = new_str;
		assert_string_eq("str-1", str_array_get(&str_a, 0));
		assert_string_eq("my string 1", str_array_get_entry(&str_a, 0)->data);

		new_str = strdup("my string 2");
		assert_nonnull(new_str);

		entry = str_array_insert(&str_a, "str0", 1);
		entry->data = new_str;
		assert_string_eq("str0", str_array_get(&str_a, 1));
		assert_string_eq("my string 2", str_array_get_entry(&str_a, 1)->data);

		new_str = strdup("my string 3");
		assert_nonnull(new_str);

		entry = str_array_insert(&str_a, "str10", 10);
		entry->data = new_str;
		assert_string_eq("str10", str_array_get(&str_a, 3));
		assert_string_eq("my string 3", str_array_get_entry(&str_a, 3)->data);
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

		str_array_insert(&str_a, "str1", 0);
		assert_string_eq("str1", str_array_get(&str_a, 0));

		str_array_insert_nodup(&str_a, str, 0);
		assert_eq(str, str_array_get(&str_a, 0));
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_sort_test)
{
	struct str_array str_a;
	str_array_init(&str_a);
	str_a.free_data = 1;

	TEST_START() {
		str_array_sort(&str_a);

		char *str1 = strdup("my string 1");
		assert_nonnull(str1);

		char *str2 = strdup("my string 2");
		assert_nonnull(str2);

		char *str3 = strdup("my string 3");
		assert_nonnull(str3);

		str_array_push(&str_a, "astr1", "dstr3", "nstr4", "bstr2", "fstr5", NULL);
		str_array_get_entry(&str_a, 1)->data = str1;
		str_array_get_entry(&str_a, 3)->data = str2;
		str_array_get_entry(&str_a, 4)->data = str3;

		str_array_sort(&str_a);
		assert_string_eq("astr1", str_array_get(&str_a, 0));
		assert_string_eq("bstr2", str_array_get(&str_a, 1));
		assert_string_eq("dstr3", str_array_get(&str_a, 2));
		assert_string_eq("fstr5", str_array_get(&str_a, 3));
		assert_string_eq("nstr4", str_array_get(&str_a, 4));

		assert_string_eq("my string 1", str_array_get_entry(&str_a, 2)->data);
		assert_string_eq("my string 2", str_array_get_entry(&str_a, 1)->data);
		assert_string_eq("my string 3", str_array_get_entry(&str_a, 3)->data);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_reverse_empty)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		// no need to assert much here; valgrind should pick up any memory weirdness
		str_array_sort(&str_a);
		assert_null(str_a.entries);
		assert_zero(str_a.len);
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_reverse_odd)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		str_array_push(&str_a, "1", NULL);
		str_array_reverse(&str_a);
		assert_string_eq("1", str_array_get(&str_a, 0));

		str_array_clear(&str_a);

		str_array_push(&str_a, "1", "2", "3", "4", "5", NULL);
		str_array_reverse(&str_a);
		assert_string_eq("5", str_array_get(&str_a, 0));
		assert_string_eq("4", str_array_get(&str_a, 1));
		assert_string_eq("3", str_array_get(&str_a, 2));
		assert_string_eq("2", str_array_get(&str_a, 3));
		assert_string_eq("1", str_array_get(&str_a, 4));
	}

	str_array_release(&str_a);
	TEST_END();
}

TEST_DEFINE(str_array_reverse_even)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		str_array_push(&str_a, "1", "2", NULL);
		str_array_reverse(&str_a);
		assert_string_eq("2", str_array_get(&str_a, 0));
		assert_string_eq("1", str_array_get(&str_a, 1));

		str_array_clear(&str_a);

		str_array_push(&str_a, "1", "2", "3", "4", NULL);
		str_array_reverse(&str_a);
		assert_string_eq("4", str_array_get(&str_a, 0));
		assert_string_eq("3", str_array_get(&str_a, 1));
		assert_string_eq("2", str_array_get(&str_a, 2));
		assert_string_eq("1", str_array_get(&str_a, 3));
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
		assert_string_eq("str2", str_array_get(&str_a, 0));
		assert_eq((ret - 1), str_a.len);

		free(removed_str);

		removed_str = str_array_remove(&str_a, 1);
		assert_string_eq("str3", removed_str);
		assert_string_eq("str4", str_array_get(&str_a, 1));
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

		assert_null(str_a.entries);
		assert_zero(str_a.len);
		assert_zero(str_a.alloc);

		assert_string_eq("str1", strings[0]);
		assert_string_eq("str5", strings[4]);
		assert_null(strings[5]);
	}

	if (strings) {
		for (size_t i = 0; i < len; i++)
			free(strings[i]);

		free(strings);
	}

	TEST_END();
}

TEST_DEFINE(str_array_detach_data_test)
{
	struct str_array str_a;
	str_array_init(&str_a);
	void **data = NULL;
	size_t len = 0;

	TEST_START() {
		str_array_push(&str_a, "str1", "str2", "str3", NULL);

		int *data1 = malloc(sizeof(int));
		assert_nonnull(data1);
		int *data2 = malloc(sizeof(int));
		assert_nonnull(data2);
		int *data3 = malloc(sizeof(int));
		assert_nonnull(data3);

		*data1 = 1;
		*data2 = 2;
		*data3 = 3;

		str_array_insert_nodup(&str_a, NULL, 0)->data = data1;
		str_array_insert_nodup(&str_a, NULL, 3)->data = data2;
		str_array_insert_nodup(&str_a, NULL, str_a.len + 100)->data = data3;

		data = str_array_detach_data(&str_a, &len);
		assert_eq_msg(6, len, "Unexpected length of str_array data array. Expected %d but was %d", 6, len);

		assert_eq(data1, data[0]);
		assert_null(data[1]);
		assert_null(data[2]);
		assert_eq(data2, data[3]);
		assert_null(data[4]);
		assert_eq(data3, data[5]);

		// the array returned should always have a size at least one larger than len
		assert_null(data[6]);
	}

	if (data) {
		for (size_t i = 0; i < len; i++)
			free(data[i]);

		free(data);
	}

	TEST_END();
}

TEST_DEFINE(str_array_clear_test)
{
	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		int ret = str_array_push(&str_a, "str1", "str2", "str3", "str4", "str5", NULL);
		assert_eq(5, ret);
		assert_eq(5, str_a.len);

		size_t alloc = str_a.alloc;
		str_array_clear(&str_a);
		assert_eq(alloc, str_a.alloc);
		assert_zero(str_a.len);
	}

	str_array_release(&str_a);
	TEST_END();
}

int str_array_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "str-array should initialize correctly", str_array_init_test },
			{ "str-array should grow in size upon request", str_array_grow_test },
			{ "str-array should release correctly", str_array_release_test },
			{ "releasing str-array with allocated data should free if free_data is non-zero", str_array_release_with_data_test },
			{ "accessing element from str-array should return correct element", str_array_get_test },
			{ "setting element in str-array should set correct element", str_array_set_test },
			{ "setting element in str-array should not free if free_data is zero", str_array_set_with_data_test },
			{ "setting element in str-array should set without duplication", str_array_set_nodup_test },
			{ "pushing elements to str-array should append to the end", str_array_push_test },
			{ "pushing elements to str-array using variadic variant should append to the end", str_array_vpush_test },
			{ "inserting element in str-array should shift elements correctly", str_array_insert_test },
			{ "inserting element in str-array should insert without duplication", str_array_insert_nodup_test },
			{ "sorting element in str-array should sort by strcmp() order", str_array_sort_test },
			{ "reversing an str-array with no elements should do nothing", str_array_reverse_empty },
			{ "reversing an str-array with an odd number of elements should reverse correctly", str_array_reverse_odd },
			{ "reversing an str-array with an even number of elements should reverse correctly", str_array_reverse_even },
			{ "removing element from str-array should shift elements correctly", str_array_remove_test },
			{ "detaching strings from str-array should correctly detach", str_array_detach_test },
			{ "detaching data from str-array should correctly detach data", str_array_detach_data_test },
			{ "clearing an str-array should remove all entries but not reallocate array", str_array_clear_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
