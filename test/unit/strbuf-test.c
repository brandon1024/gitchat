#include <unistd.h>

#include "test-lib.h"
#include "strbuf.h"
#include "utils.h"

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

		strbuf_attach_fmt(&buf, "%s %s %d", "Hello", "World", 1);
		assert_string_eq("Hello World 1Hello World 1", buf.buff);
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
		string[0] = '%';
		string[1] = 's';
		string[127] = 0;

		strbuf_attach_fmt(&buf, string, "HITHERE");
		assert_eq(132, buf.len);
		assert_true(is_null_terminated(&buf));
	}

	strbuf_release(&buf);
	TEST_END();
}

TEST_DEFINE(strbuf_attach_fd_test) {
	struct strbuf buf;
	strbuf_init(&buf);

	int fd[2];

	TEST_START() {
		assert_false_msg(pipe(fd) < 0, "pipe allocation failed");

		// happy path
		const char *str = "my string message";
		size_t str_len = strlen(str);
		assert_eq_msg(str_len, xwrite(fd[1], str, str_len), "pipe write failed");
		close(fd[1]);

		strbuf_attach_fd(&buf, fd[0]);
		assert_string_eq(str, buf.buff);
	}

	strbuf_release(&buf);
	close(fd[0]);

	TEST_END();
}

TEST_DEFINE(strbuf_attach_fd_stop_first_null_test) {
	struct strbuf buf;
	strbuf_init(&buf);

	int fd[2];

	TEST_START() {
		assert_false_msg(pipe(fd) < 0, "pipe allocation failed");

		// happy path
		const char *str = "my string message\0hello world\0hi";
		size_t str_len = 32;
		assert_eq_msg(str_len, xwrite(fd[1], str, str_len), "pipe write failed");
		close(fd[1]);

		strbuf_attach_fd(&buf, fd[0]);
		assert_string_eq("my string message", buf.buff);
	}

	strbuf_release(&buf);
	close(fd[0]);

	TEST_END();
}

TEST_DEFINE(strbuf_trim_test) {
	struct strbuf buf;
	strbuf_init(&buf);

	int ret;
	TEST_START() {
		//entirely whitespace
		char *str = "  \t    \r\v  \n \f";
		strbuf_attach_str(&buf, str);
		assert_zero(buf.buff[buf.len]);
		ret = strbuf_trim(&buf);
		assert_eq_msg(strlen(str), (size_t)ret, "Incorrect number of characters trimmed from strbuf.");
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

		assert_string_eq(str_1, str_a.entries[0].string);
		assert_string_eq(str_2, str_a.entries[1].string);
		assert_string_eq(str_3, str_a.entries[2].string);
		assert_string_eq("", str_a.entries[3].string);
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

		assert_string_eq("This is a more complex ", str_a.entries[0].string);
		assert_string_eq("ing testing ", str_a.entries[1].string);
		assert_string_eq("buf splitting with multichar delimiters.", str_a.entries[2].string);

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
		assert_string_eq(string, str_a.entries[0].string);

		str_array_release(&str_a);
		str_array_init(&str_a);

		ret = strbuf_split(&buf, "", &str_a);
		assert_eq_msg(1, ret, "strbuf should have inserted single string to str_array but inserted %d.", ret);
		assert_eq_msg(1, str_a.len, "str_array should have a length of 1 but was %d", str_a.len);
		assert_string_eq(string, str_a.entries[0].string);
	}

	str_array_release(&str_a);
	strbuf_release(&buf);

	TEST_END();
}

TEST_DEFINE(strbuf_remove_beyond_buffer_length_test)
{
	struct strbuf sb;
	strbuf_init(&sb);

	TEST_START() {
		const char *buffer = "expected buffer!";
		strbuf_attach_str(&sb, buffer);

		size_t len = sb.len;
		size_t alloc = sb.alloc;
		strbuf_remove(&sb, sb.len, sb.len);

		assert_eq(len, sb.len);
		assert_eq(alloc, sb.alloc);
		assert_string_eq(buffer, sb.buff);
	}

	strbuf_release(&sb);

	TEST_END();
}

TEST_DEFINE(strbuf_remove_from_beginning_test)
{
	struct strbuf sb;
	strbuf_init(&sb);

	TEST_START() {
		char buffer[] = "expected buffer!";
		strbuf_attach_str(&sb, buffer);

		size_t len = sb.len;
		size_t alloc = sb.alloc;
		strbuf_remove(&sb, 0, 8);

		assert_eq(len - 8, sb.len);
		assert_eq(alloc, sb.alloc);
		assert_string_eq(" buffer!", sb.buff);
	}

	strbuf_release(&sb);

	TEST_END();
}

TEST_DEFINE(strbuf_remove_from_within_test)
{
	struct strbuf sb;
	strbuf_init(&sb);

	TEST_START() {
		char buffer[] = "expected string buffer!";
		strbuf_attach_str(&sb, buffer);

		size_t len = sb.len;
		size_t alloc = sb.alloc;
		strbuf_remove(&sb, 9, 6);

		assert_eq(len - 6, sb.len);
		assert_eq(alloc, sb.alloc);
		assert_string_eq("expected  buffer!", sb.buff);
	}

	strbuf_release(&sb);

	TEST_END();
}

TEST_DEFINE(strbuf_remove_length_larger_than_buffer_length_test)
{
	struct strbuf sb;
	strbuf_init(&sb);

	TEST_START() {
		char buffer[] = "expected string buffer!";
		strbuf_attach_str(&sb, buffer);

		size_t alloc = sb.alloc;
		strbuf_remove(&sb, 9, 100);

		assert_eq(9, sb.len);
		assert_eq(alloc, sb.alloc);
		assert_eq(0, sb.buff[9]);
		assert_string_eq("expected ", sb.buff);
	}

	strbuf_release(&sb);

	TEST_END();
}

TEST_DEFINE(strbuf_clear_test)
{
	struct strbuf buf;
	strbuf_init(&buf);

	TEST_START() {
		strbuf_attach_str(&buf, "hello");
		strbuf_attach_str(&buf, "world");
		assert_eq(10, buf.len);
		size_t allocated = buf.alloc;

		strbuf_clear(&buf);
		assert_zero(buf.len);
		assert_eq(allocated, buf.alloc);
		assert_string_eq("", buf.buff);
	}

	strbuf_release(&buf);

	TEST_END();
}

const char *suite_name = SUITE_NAME;
int test_suite(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "strbuf should initialize correctly", strbuf_init_test },
			{ "strbuf should release correctly", strbuf_release_test },
			{ "resizing a strbuf should resize as expected", strbuf_grow_test },
			{ "attaching string to strbuf should grow the strbuf appropriately", strbuf_attach_test },
			{ "attaching character to strbuf should grow the strbuf appropriately", strbuf_attach_chr_test },
			{ "attaching a formatted string to strbuf should format the buffer correctly", strbuf_attach_fmt_test },
			{ "attaching string to a strbuf from a file descriptor should consume data from file descriptor", strbuf_attach_fd_test },
			{ "attaching string to a strbuf from a file descriptor should consume data up to the first null byte", strbuf_attach_fd_stop_first_null_test },
			{ "trimming whitespace from strbuf should trim correct number of characters", strbuf_trim_test },
			{ "detaching string from strbuf should return correct string", strbuf_detach_test },
			{ "splitting a strbuf on a simple delimiter should split as expected", strbuf_split_simple_delim_test },
			{ "splitting a strbuf on a multi-character delimiter should split as expected", strbuf_split_multichar_delim_test },
			{ "splitting a strbuf without a delimiter should push entire string to str_array", strbuf_split_no_delim_test },
			{ "removing data from beyond the strbuf buffer length should have no effect", strbuf_remove_beyond_buffer_length_test },
			{ "removing data from beginning of strbuf should shift all bytes to the left", strbuf_remove_from_beginning_test },
			{ "removing data from within the strbuf should remove inner portion correctly", strbuf_remove_from_within_test },
			{ "removing data from strbuf with length larger than buffer should simply remove remaining bytes", strbuf_remove_length_larger_than_buffer_length_test },
			{ "clearing content of a strbuf should not resize buffer but clear content", strbuf_clear_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
