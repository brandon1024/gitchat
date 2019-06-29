#include "test-lib.h"
#include "strbuf.h"

TEST_DEFINE(strbuf_init_test)
{
	struct strbuf buf;
	strbuf_init(&buf);

	TEST_START() {
		assert_zero(buf.len);
		assert_nonnull(buf.buff);
		assert_string_eq("", buf.buff);
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

		/* attach string with strlen(str) + 10 for buffer length */
		attached_str = " This is also a test!";
		expected_str = "This is a test! This is also a test!";
		strbuf_attach(&buf, attached_str, strlen(attached_str) + 10);
		assert_string_eq(expected_str, buf.buff);
		assert_eq(strlen(buf.buff), buf.len);

		/* attach portion of string */
		attached_str = " This is also a test!";
		expected_str = "This is a test! This is also a test! This is ";
		strbuf_attach(&buf, attached_str, 9);
		assert_string_eq(expected_str, buf.buff);
		assert_eq(strlen(buf.buff), buf.len);
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

int strbuf_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "strbuf should initialize correctly", strbuf_init_test },
			{ "strbuf should release correctly", strbuf_release_test },
			{ "resizing a strbuf should resize as expected", strbuf_grow_test },
			{ "attaching string to strbuf should grow the strbuf appropriately", strbuf_attach_test },
			{ "attaching character to strbuf should grow the strbuf appropriately", strbuf_attach_chr_test },
			{ "detaching string from strbuf should return correct string", strbuf_detach_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
