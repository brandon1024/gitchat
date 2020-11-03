#include "test-lib.h"
#include "config/config-key.h"

static const char * const valid_keys[] = {
		"test",
		"test.property",
		"test.property.section_1",
		"test.property.1",
		"test.property.1_section",
		"\"test\".property.section",
		"\"test-section\".property.section",
		"\"test-s\\\"ection\".property.section",
		"test.\"pro-.-perty\".section",
		"test.property.\"secti on -\"",
		"\" a.b.c \".\"     \".\"  \\n  \\\"  \"",
		NULL
};

static const char * const invalid_keys[] = {
		"",
		".test",
		"test.",
		"test.\"unfinished",
		"test.'ropert'y.section",
		"test.p\"ropert\"y.section",
		"test.\"prope\"rty.section",
		"test.unfinished\"",
		"test..unfinished.test1",
		"test unfinished.test1",
		"test.unfinishedtest-1",
		"test. unfinishedtest1",
		"test.unfinishedtest1 ",
		"test.unfinished .test1",
		"test.unfinished. test1",
		"test.\"unfi\tnished\".test1",
		NULL
};

TEST_DEFINE(is_valid_config_key_detect_valid_test)
{
	TEST_START() {
		for (const char * const *key = valid_keys; *key; key++) {
			int is_valid = is_valid_config_key(*key);
			assert_true_msg(is_valid, "key '%s' should be valid", *key);
		}
	}

	TEST_END();
}

TEST_DEFINE(is_valid_config_key_detect_invalid_test)
{
	TEST_START() {
		for (const char * const *key = invalid_keys; *key; key++) {
			int is_valid = is_valid_config_key(*key);
			assert_false_msg(is_valid, "key '%s' should be invalid", *key);
		}
	}

	TEST_END();
}

TEST_DEFINE(isolate_config_key_components_happy_path_test)
{
	struct str_array components;
	str_array_init(&components);

	const char *key = "this.is.a.pretty.simple.key";

	TEST_START() {
		int status = isolate_config_key_components(key, &components);
		assert_zero_msg(status, "isolating key components should pass for simple key '%s'", key);

		// verify components
		assert_eq_msg(6, components.len, "unexpected number of key components");
		assert_string_eq_msg("this", str_array_get(&components, 0), "component 0 is unexpected");
		assert_string_eq_msg("is", str_array_get(&components, 1), "component 1 is unexpected");
		assert_string_eq_msg("a", str_array_get(&components, 2), "component 2 is unexpected");
		assert_string_eq_msg("pretty", str_array_get(&components, 3), "component 3 is unexpected");
		assert_string_eq_msg("simple", str_array_get(&components, 4), "component 4 is unexpected");
		assert_string_eq_msg("key", str_array_get(&components, 5), "component 5 is unexpected");

		// should push to end of components array
		str_array_clear(&components);
		str_array_push(&components, "my string", NULL);
		status = isolate_config_key_components("simple.key", &components);
		assert_zero_msg(status, "isolating key components should pass for simple key '%s'", key);

		assert_eq_msg(3, components.len, "unexpected number of key components");
		assert_string_eq_msg("my string", str_array_get(&components, 0), "component 0 is unexpected");
		assert_string_eq_msg("simple", str_array_get(&components, 1), "component 1 is unexpected");
		assert_string_eq_msg("key", str_array_get(&components, 2), "component 2 is unexpected");
	}

	str_array_release(&components);

	TEST_END();
}

TEST_DEFINE(isolate_config_key_components_quoted_components_test)
{
	struct str_array components;
	str_array_init(&components);

	const char * quoted_keys[] = {
			"\"this\".key.is.quoted",
			"this.\"key\".is.quoted",
			"this.key.\"is\".quoted",
			"this.key.is.\"quoted\"",
			"\"this\".key.is.\"quoted\"",
			"this.\"key\".\"is\".quoted",
			"\"this\".\"key\".\"is\".\"quoted\"",
			NULL,
	};

	TEST_START() {
		for (const char **key = quoted_keys; *key; key++) {
			int status = isolate_config_key_components(*key, &components);
			assert_zero_msg(status, "failed to isolate key components for key '%s'", *key);

			assert_eq_msg(4, components.len, "unexpected number of key components");
			assert_string_eq_msg("this", str_array_get(&components, 0), "component 0 is unexpected");
			assert_string_eq_msg("key", str_array_get(&components, 1), "component 1 is unexpected");
			assert_string_eq_msg("is", str_array_get(&components, 2), "component 2 is unexpected");
			assert_string_eq_msg("quoted", str_array_get(&components, 3), "component 3 is unexpected");

			str_array_clear(&components);
		}
	}

	str_array_release(&components);

	TEST_END();
}

TEST_DEFINE(isolate_config_key_components_single_component_test)
{
	struct str_array components;
	str_array_init(&components);

	const char *key1 = "onesinglecomponent";

	TEST_START() {
		int status = isolate_config_key_components(key1, &components);
		assert_zero_msg(status, "failed to isolate key components for key '%s'", key1);
		assert_eq_msg(1, components.len, "unexpected number of key components");
		assert_string_eq(key1, str_array_get(&components, 0));
	}

	str_array_release(&components);

	TEST_END();
}

TEST_DEFINE(isolate_config_key_components_escapes_test)
{
	struct str_array components;
	str_array_init(&components);

	const char *key1 = "\"tes\\t\".key";
	const char *key2 = "test.\"k\\\"ey\".quotes";
	const char *key3 = "test.key.\"quo\\\\tes\"";

	TEST_START() {
		// escape arbitrary character
		int status = isolate_config_key_components(key1, &components);
		assert_zero_msg(status, "failed to isolate key components for key '%s'", key1);
		assert_eq_msg(2, components.len, "unexpected number of key components");
		assert_string_eq("test", str_array_get(&components, 0));
		str_array_clear(&components);

		// escape quotes
		status = isolate_config_key_components(key2, &components);
		assert_zero_msg(status, "failed to isolate key components for key '%s'", key2);
		assert_eq_msg(3, components.len, "unexpected number of key components");
		assert_string_eq("k\"ey", str_array_get(&components, 1));
		str_array_clear(&components);

		// escape escape character
		status = isolate_config_key_components(key3, &components);
		assert_zero_msg(status, "failed to isolate key components for key '%s'", key3);
		assert_eq_msg(3, components.len, "unexpected number of key components");
		assert_string_eq("quo\\tes", str_array_get(&components, 2));
	}

	str_array_release(&components);

	TEST_END();
}

TEST_DEFINE(isolate_config_key_components_invalid_key_test)
{
	struct str_array components;
	str_array_init(&components);

	TEST_START() {
		for (const char * const *key = invalid_keys; *key; key++) {
			int is_valid = isolate_config_key_components(*key, &components);
			assert_nonzero_msg(is_valid, "key '%s' should be invalid", *key);
			assert_zero_msg(components.len, "passing invalid key to isolate_config_key_components should not mutate string array");
		}
	}

	str_array_release(&components);

	TEST_END();
}

TEST_DEFINE(merge_config_key_components_happy_path_test)
{
	struct strbuf key;
	struct str_array components;

	TEST_START() {
		strbuf_init(&key);
		str_array_init(&components);

		str_array_push(&components, "simplekey", NULL);
		int status = merge_config_key_components(&components, &key);
		assert_zero_msg(status, "failed to merge simple single key component");
		assert_string_eq("simplekey", key.buff);
		strbuf_clear(&key);
		str_array_clear(&components);

		str_array_push(&components, "this", "is", "a", "long", "key", NULL);
		status = merge_config_key_components(&components, &key);
		assert_zero_msg(status, "failed to merge key components");
		assert_string_eq("this.is.a.long.key", key.buff);
	}

	str_array_release(&components);
	strbuf_release(&key);

	TEST_END();
}

TEST_DEFINE(merge_config_key_components_quoted_test)
{
	struct strbuf key;
	struct str_array components;

	TEST_START() {
		strbuf_init(&key);
		str_array_init(&components);

		str_array_push(&components, "simp-lekey", NULL);
		int status = merge_config_key_components(&components, &key);
		assert_zero_msg(status, "failed to merge single key component with symbols");
		assert_string_eq("\"simp-lekey\"", key.buff);
		strbuf_clear(&key);
		str_array_clear(&components);

		str_array_push(&components, "th-is", "i.s", "ano_ther", "lo\"ng", "ke'y", NULL);
		status = merge_config_key_components(&components, &key);
		assert_zero_msg(status, "failed to merge key components");
		assert_string_eq("\"th-is\".\"i.s\".ano_ther.\"lo\\\"ng\".\"ke'y\"", key.buff);
	}

	str_array_release(&components);
	strbuf_release(&key);

	TEST_END();
}

TEST_DEFINE(merge_config_key_components_escape_test)
{
	struct strbuf key;
	struct str_array components;

	TEST_START() {
		strbuf_init(&key);
		str_array_init(&components);

		str_array_push(&components, "this", "ke\"y", "\"contains\"", "quotes", NULL);
		int status = merge_config_key_components(&components, &key);
		assert_zero_msg(status, "failed to merge single key component with symbols");
		assert_string_eq("this.\"ke\\\"y\".\"\\\"contains\\\"\".quotes", key.buff);
	}

	str_array_release(&components);
	strbuf_release(&key);

	TEST_END();
}

TEST_DEFINE(merge_config_key_components_no_components_test)
{
	struct strbuf key;
	struct str_array components;

	TEST_START() {
		strbuf_init(&key);
		str_array_init(&components);

		int status = merge_config_key_components(&components, &key);
		assert_eq_msg(1, status, "expected key component merge to fail when given no components");
		assert_zero_msg(key.len, "key buffer should not be updated when merge fails");
	}

	str_array_release(&components);
	strbuf_release(&key);

	TEST_END();
}

TEST_DEFINE(isolate_merge_normalization_test)
{
	struct strbuf key_buff;
	struct str_array components;

	const char *key = "\" a.b.c \".\"     \".something.\"  \\n  \\\"  \".wow_zers";
	const char *key_normalized = "\" a.b.c \".\"     \".something.\"  n  \\\"  \".wow_zers";

	TEST_START() {
		strbuf_init(&key_buff);
		str_array_init(&components);

		// test isolate -> merge to verify that key is normalized
		int status = isolate_config_key_components(key, &components);
		assert_zero_msg(status, "failed to isolate key components");
		status = merge_config_key_components(&components, &key_buff);
		assert_zero_msg(status, "failed to merge key components");
		assert_string_eq(key_normalized, key_buff.buff);
	}

	str_array_release(&components);
	strbuf_release(&key_buff);

	TEST_END();
}

const char *suite_name = SUITE_NAME;
int test_suite(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "is_valid_config_key should detect valid keys", is_valid_config_key_detect_valid_test },
			{ "is_valid_config_key should detect invalid keys", is_valid_config_key_detect_invalid_test },
			{ "splitting key into components should work as expected for basic keys", isolate_config_key_components_happy_path_test },
			{ "splitting keys with quoted components should unquote", isolate_config_key_components_quoted_components_test },
			{ "splitting keys with single component should only insert single key into string array", isolate_config_key_components_single_component_test },
			{ "splitting keys should remove character escapes before insertion into string array", isolate_config_key_components_escapes_test },
			{ "supplying invalid keys to isolate_config_key_components should fail", isolate_config_key_components_invalid_key_test },
			{ "merging simple key components should produce expected key strings", merge_config_key_components_happy_path_test },
			{ "key components with symbols should be quoted when merged", merge_config_key_components_quoted_test },
			{ "double quotes in key components should be escaped before merged", merge_config_key_components_escape_test },
			{ "attempting to merge an array with no components should fail", merge_config_key_components_no_components_test },
			{ "isolating and then merging a key should normalize its key components", isolate_merge_normalization_test },
			{ NULL, NULL },
	};

	return execute_tests(instance, tests);
}
