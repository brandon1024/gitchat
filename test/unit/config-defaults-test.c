#include "test-lib.h"
#include "config/config-defaults.h"

TEST_DEFINE(config_default_get_default_value_nonexistent)
{
	TEST_START() {
		const char *value = get_default_config_value("channel.name");
		assert_null(value);
		value = get_default_config_value("channel.testme.name1");
		assert_null(value);
		value = get_default_config_value("channel.a.a.name");
		assert_null(value);
		value = get_default_config_value("achannel.test.name");
		assert_null(value);
	}

	TEST_END();
}

TEST_DEFINE(config_default_get_default_value_matching)
{
	TEST_START() {
		const char *value = get_default_config_value("channel.master.name");
		assert_string_eq("", value);
		value = get_default_config_value("channel.master123.name");
		assert_string_eq("", value);
		value = get_default_config_value("channel.a.name");
		assert_string_eq("", value);
	}

	TEST_END();
}

TEST_DEFINE(is_recognized_key_test)
{
	TEST_START() {
		assert_nonzero(is_recognized_config_key("channel.test.createdby"));
		assert_nonzero(is_recognized_config_key("channel.testme.name"));
		assert_nonzero(is_recognized_config_key("channel.\"testme\".name"));
		assert_nonzero(is_recognized_config_key("channel.\"test.me\".name"));
		assert_zero(is_recognized_config_key("unknown"));
		assert_zero(is_recognized_config_key("channel.test.test.name"));
		assert_zero(is_recognized_config_key("channel. invalid .createdby"));
	}

	TEST_END();
}

int config_defaults_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "unrecognized config should return null", config_default_get_default_value_nonexistent },
			{ "get_default_config_value should correcly match key against pattern", config_default_get_default_value_matching },
			{ "is_recognized_config_key should follow similar pattern-matching to get_default_config_value", is_recognized_key_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
