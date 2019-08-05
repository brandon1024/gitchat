#include "test-lib.h"
#include "strbuf.h"

static int is_null_terminated(struct strbuf *buff)
{
	return buff->buff[buff->len] == 0;
}

TEST_DEFINE(strbuf_init_test)
{
	struct strbuf buf;
	strbuf_init(&buf);

	TEST_START() {
		assert_zero(buf.len);
		assert_nonnull(buf.buff);
		assert_string_eq("", buf.buff);
		assert_true(is_null_terminated(&buf));
	}

	strbuf_release(&buf);
	TEST_END();
}

TEST_DEFINE(strbuf_release_test)
{
	struct strbuf buf;
	strbuf_init(&buf);
	strbuf_release(&buf);

	TEST_START() {
		assert_zero(buf.len);
		assert_null(buf.buff);
	}

	TEST_END();
}

TEST_DEFINE(strbuf_grow_test)
{
	struct strbuf buf;
	strbuf_init(&buf);

	TEST_START() {
		size_t len = buf.alloc + 256;
		strbuf_grow(&buf, len);
		assert_eq(len, buf.alloc);
		assert_true(is_null_terminated(&buf));

		strbuf_grow(&buf, 128);
		assert_eq(len, buf.alloc);
		assert_true(is_null_terminated(&buf));
	}

	strbuf_release(&buf);

	TEST_END();
}

TEST_DEFINE(strbuf_attach_test)
{
	struct strbuf buf;
	strbuf_init(&buf);

	TEST_START() {
		/* attach string with strlen(str) for buffer length */
		char *attached_str = "This is a test!";
		char *expected_str = "This is a test!";
		strbuf_attach(&buf, attached_str, strlen(attached_str));
		assert_string_eq(expected_str, buf.buff);
		assert_eq(strlen(buf.buff), buf.len);
		assert_true(is_null_terminated(&buf));

		/* attach string with strlen(str) + 10 for buffer length */
		attached_str = " This is also a test!";
		expected_str = "This is a test! This is also a test!";
		strbuf_attach(&buf, attached_str, strlen(attached_str) + 10);
		assert_string_eq(expected_str, buf.buff);
		assert_eq(strlen(buf.buff), buf.len);
		assert_true(is_null_terminated(&buf));

		/* attach portion of string */
		attached_str = " This is also a test!";
		expected_str = "This is a test! This is also a test! This is ";
		strbuf_attach(&buf, attached_str, 9);
		assert_string_eq(expected_str, buf.buff);
		assert_eq(strlen(buf.buff), buf.len);
		assert_true(is_null_terminated(&buf));
	}

	strbuf_release(&buf);
	TEST_END();
}

TEST_DEFINE(strbuf_attach_chr_test)
{
	struct strbuf buf;
	strbuf_init(&buf);
	
	TEST_START() {
		strbuf_attach_chr(&buf, 'f');
		strbuf_attach_chr(&buf, 'o');
		strbuf_attach_chr(&buf, 'o');
		assert_string_eq("foo", buf.buff);
		assert_true(is_null_terminated(&buf));
	}
	
	strbuf_release(&buf);
	TEST_END();
}

TEST_DEFINE(strbuf_attach_fmt_test)
{
	struct strbuf buf;
	strbuf_init(&buf);

	TEST_START() {
		strbuf_attach_fmt(&buf, "%s %s %d", "Hello", "World", 1);
		assert_string_eq("Hello World 1", buf.buff);
		assert_true(is_null_terminated(&buf));

		strbuf_release(&buf);
		strbuf_init(&buf);

		strbuf_attach_fmt(&buf, "");
		assert_string_eq("", buf.buff);
		assert_true(is_null_terminated(&buf));

		strbuf_release(&buf);
		strbuf_init(&buf);

		char string[128];
		memset(string, 'A', 128);
		string[127] = 0;
		string[0] = '%';
		string[1] = 's';

		strbuf_attach_fmt(&buf, string, "HITHERE");
		assert_eq(132, buf.len);
		assert_true(is_null_terminated(&buf));
	}

	strbuf_release(&buf);
	TEST_END();
}

TEST_DEFINE(strbuf_trim_test) {
	struct strbuf buf;
	strbuf_init(&buf);

	int ret = 0;
	TEST_START() {
		//entirely whitespace
		char *str = "  \t    \r\v  \n \f";
		strbuf_attach_str(&buf, str);
		assert_zero(buf.buff[buf.len]);
		ret = strbuf_trim(&buf);
		assert_eq_msg(strlen(str), ret, "Incorrect number of characters trimmed from strbuf.");
		assert_zero(buf.len);
		assert_true(is_null_terminated(&buf));

		strbuf_release(&buf);
		strbuf_init(&buf);

		//leading whitespace
		strbuf_attach_str(&buf, " \t\nHello World!");
		assert_zero(buf.buff[buf.len]);
		ret = strbuf_trim(&buf);
		assert_eq_msg(3, ret, "Incorrect number of characters trimmed from strbuf.");
		assert_eq(12, buf.len);
		assert_string_eq("Hello World!", buf.buff);
		assert_true(is_null_terminated(&buf));

		strbuf_release(&buf);
		strbuf_init(&buf);

		//trailing whitespace
		strbuf_attach_str(&buf, "Hello World! \t\n");
		assert_zero(buf.buff[buf.len]);
		ret = strbuf_trim(&buf);
		assert_eq_msg(3, ret, "Incorrect number of characters trimmed from strbuf.");
		assert_eq(12, buf.len);
		assert_string_eq("Hello World!", buf.buff);
		assert_true(is_null_terminated(&buf));

		strbuf_release(&buf);
		strbuf_init(&buf);

		//leading and trailing whitespace
		strbuf_attach_str(&buf, "\f\r\vHello World! \t\n");
		assert_zero(buf.buff[buf.len]);
		ret = strbuf_trim(&buf);
		assert_eq_msg(6, ret, "Incorrect number of characters trimmed from strbuf.");
		assert_eq(12, buf.len);
		assert_string_eq("Hello World!", buf.buff);
		assert_true(is_null_terminated(&buf));

		strbuf_release(&buf);
		strbuf_init(&buf);

		//buffer without null terminator
		strbuf_attach_str(&buf, "   Hello World!   123   ");
		buf.alloc = buf.len = 18;
		ret = strbuf_trim(&buf);
		assert_eq_msg(6, ret, "Incorrect number of characters trimmed from strbuf.");
		assert_eq(12, buf.len);
		assert_string_eq("Hello World!", buf.buff);
		assert_true(is_null_terminated(&buf));
	}

	strbuf_release(&buf);

	TEST_END();
}

TEST_DEFINE(strbuf_detach_test)
{
	struct strbuf buf;
	strbuf_init(&buf);
	char *detached_str = NULL;

	TEST_START() {
		/* attach string with strlen(str) for buffer length */
		char *expected_str = "0123456789abcdef";
		char *attached_str = "0123";
		strbuf_attach(&buf, attached_str, strlen(attached_str));
		attached_str = "4567";
		strbuf_attach(&buf, attached_str, strlen(attached_str));
		attached_str = "89ab";
		strbuf_attach(&buf, attached_str, strlen(attached_str));
		attached_str = "cdef";
		strbuf_attach(&buf, attached_str, strlen(attached_str));

		detached_str = strbuf_detach(&buf);
		assert_zero(buf.len);
		assert_string_eq(expected_str, detached_str);
	}

	free(detached_str);
	strbuf_release(&buf);
	TEST_END();
}

TEST_DEFINE(strbuf_split_simple_delim_test)
{
	struct strbuf buf;
	strbuf_init(&buf);

	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		const char *str_1 = "this ";
		const char *str_2 = " is a";
		const char *str_3 = "test\tbuffer";
		strbuf_attach_fmt(&buf, "%s\n%s\n%s\n", str_1, str_2, str_3);

		int ret = strbuf_split(&buf, "\n", &str_a);
		assert_eq_msg(4, ret, "strbuf should have split into 4 strings but was %d.", ret);
		assert_eq_msg(4, str_a.len, "str_array should have a length of 4 but was %d", str_a.len);
		
		assert_string_eq(str_1, str_a.strings[0]);
		assert_string_eq(str_2, str_a.strings[1]);
		assert_string_eq(str_3, str_a.strings[2]);
		assert_string_eq("", str_a.strings[3]);
	}

	strbuf_release(&buf);
	str_array_release(&str_a);

	TEST_END();
}

TEST_DEFINE(strbuf_split_multichar_delim_test)
{
	struct strbuf buf;
	strbuf_init(&buf);

	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		const char *string =
				"This is a more complex string "
				"testing strbuf splitting with "
				"multichar delimiters.";
		strbuf_attach_str(&buf, string);

		int ret = strbuf_split(&buf, "str", &str_a);
		assert_eq_msg(3, ret, "strbuf should have split into 3 strings but was %d.", ret);
		assert_eq_msg(3, str_a.len, "str_array should have a length of 3 but was %d", str_a.len);

		assert_string_eq("This is a more complex ", str_a.strings[0]);
		assert_string_eq("ing testing ", str_a.strings[1]);
		assert_string_eq("buf splitting with multichar delimiters.", str_a.strings[2]);

		assert_string_eq_msg(string, buf.buff, "strbuf_split should not modify the strbuf, but content did not match expected string.");
	}

	str_array_release(&str_a);
	strbuf_release(&buf);

	TEST_END();
}

TEST_DEFINE(strbuf_split_no_delim_test)
{
	struct strbuf buf;
	strbuf_init(&buf);

	struct str_array str_a;
	str_array_init(&str_a);

	TEST_START() {
		const char *string =
				"This is a more complex string "
				"testing strbuf splitting with "
				"multichar delimiters.";
		strbuf_attach_str(&buf, string);

		int ret = strbuf_split(&buf, NULL, &str_a);
		assert_eq_msg(1, ret, "strbuf should have inserted single string to str_array but inserted %d.", ret);
		assert_eq_msg(1, str_a.len, "str_array should have a length of 1 but was %d", str_a.len);
		assert_string_eq(string, str_a.strings[0]);

		str_array_release(&str_a);
		str_array_init(&str_a);

		ret = strbuf_split(&buf, "", &str_a);
		assert_eq_msg(1, ret, "strbuf should have inserted single string to str_array but inserted %d.", ret);
		assert_eq_msg(1, str_a.len, "str_array should have a length of 1 but was %d", str_a.len);
		assert_string_eq(string, str_a.strings[0]);
	}

	str_array_release(&str_a);
	strbuf_release(&buf);

	TEST_END();
}

int strbuf_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "strbuf should initialize correctly", strbuf_init_test },
			{ "strbuf should release correctly", strbuf_release_test },
			{ "resizing a strbuf should resize as expected", strbuf_grow_test },
			{ "attaching string to strbuf should grow the strbuf appropriately", strbuf_attach_test },
			{ "attaching character to strbuf should grow the strbuf appropriately", strbuf_attach_chr_test },
			{ "attaching a formatted string to strbuf should format the buffer correctly", strbuf_attach_fmt_test },
			{ "trimming whitespace from strbuf should trim correct number of characters", strbuf_trim_test },
			{ "detaching string from strbuf should return correct string", strbuf_detach_test },
			{ "splitting a strbuf on a simple delimiter should split as expected", strbuf_split_simple_delim_test },
			{ "splitting a strbuf on a multi-character delimiter should split as expected", strbuf_split_multichar_delim_test },
			{ "splitting a strbuf without a delimiter should push entire string to str_array", strbuf_split_no_delim_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
