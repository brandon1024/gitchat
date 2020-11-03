#include "test-lib.h"
#include "config/config-data.h"

static const char * const valid_keys[] = {
		"test",
		"test.property",
		"test.property.section_1",
		"test.property.1",
		"test.property.1_section",
		"\"test\".property.section1",
		"test.\"prop-erty\".section2",
		"test.property.\"is-key\"",
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

TEST_DEFINE(config_data_init_test)
{
	struct config_data *config = NULL;

	TEST_START() {
		config_data_init(&config);

		assert_null_msg(config->parent, "config_data parent should be null");
		assert_null_msg(config->section, "config_data section should be null");
		assert_zero_msg(config->subsections_len, "config_data entries array length should be zero");
		assert_eq_msg(8, config->subsections_alloc, "config_data entries array should be initialized with a size of 8");
		assert_nonnull_msg(config->subsections, "config_data subsections array should not be null");

		config_data_release(&config);
		assert_null_msg(config, "config_data_release should null reference");
	}

	if (config)
		config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_insert_invalid_key_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		for (const char * const *k = invalid_keys; *k; k++) {
			const char *invalid_key = *k;

			int status = config_data_insert(config, invalid_key, "my value");
			assert_eq_msg(-1, status, "config_data_insert should return -1 when key is invalid");
		}
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_insert_duplicate_fail_test)
{
	struct config_data *config;

	const char *key = "this.is.my.key";

	TEST_START() {
		config_data_init(&config);

		int status = config_data_insert(config, key, "my value");
		assert_zero_msg(status, "config_data_insert should successfully insert key '%s'", key);

		status = config_data_insert(config, key, "my value");
		assert_eq_msg(1, status, "config_data_insert fail when trying to insert duplicate key '%s'", key);
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_insert_valid_key_pass_test)
{
	struct config_data *config;

	const char *expected_value = "my value";

	TEST_START() {
		config_data_init(&config);

		for (const char * const *k = valid_keys; *k; k++) {
			const char *valid_key = *k;

			int status = config_data_insert(config, valid_key, expected_value);
			assert_zero_msg(status, "config_data_insert should successfully insert key '%s'", valid_key);

			const char *value = config_data_find(config, valid_key);
			assert_string_eq_msg(expected_value, value, "failed to retrieve value for valid key '%s'", valid_key);
		}
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_update_invalid_key_fail_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		for (const char * const *k = invalid_keys; *k; k++) {
			const char *invalid_key = *k;

			int status = config_data_update(config, invalid_key, "my value");
			assert_eq_msg(-1, status, "config_data_update should return -1 when key is invalid");
		}
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_update_no_create_paths_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		int status = config_data_update(config, "this.key.does.not.exist", "my value");
		assert_eq_msg(1, status, "config_data_update should return 1 when key is missing");

		assert_zero_msg(config->subsections_len, "config_data_update should not create additional subsections");
		assert_zero_msg(config->entries.len, "config_data_update should not create additional entries");
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_update_missing_section_fails_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		int status = config_data_update(config, "this.key.does.not.exist", "my value");
		assert_eq_msg(1, status, "config_data_update should return 1 when key is missing");
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_update_missing_property_fails_test)
{
	struct config_data *config;

	const char *key = "this.key.will.exist";
	const char *key1 = "this.key.will.exist1";

	TEST_START() {
		config_data_init(&config);

		int status = config_data_insert(config, key, "my value");
		assert_zero_msg(status, "config_data_insert should create property for key '%s'", key);

		status = config_data_update(config, key1, "my_value");
		assert_eq_msg(1, status, "config_data_update should return 1 for key '%s'", key1);

		const char *property = config_data_find(config, key);
		assert_nonnull_msg(property, "should successfully locate property with key '%s'", key);
		assert_string_eq_msg("my value", property, "should successfully locate property with key '%s'", key);

		property = config_data_find(config, key1);
		assert_null_msg(property, "should not locate property with key '%s'", key1);
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_update_valid_key_pass_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		for (const char * const *k = valid_keys; *k; k++) {
			const char *valid_key = *k;

			int status = config_data_insert(config, valid_key, "my value");
			assert_zero_msg(status, "config_data_insert should successfully insert key '%s'", valid_key);
		}

		for (const char * const *k = valid_keys; *k; k++) {
			const char *valid_key = *k;

			int status = config_data_update(config, valid_key, "my new value");
			assert_zero_msg(status, "config_data_update should successfully update key '%s'", valid_key);

			const char *property = config_data_find(config, valid_key);
			assert_nonnull_msg(property, "should successfully locate property with key '%s'", valid_key);
			assert_string_eq_msg("my new value", property, "should successfully locate property with key '%s'", valid_key);
		}
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_delete_invalid_key_fail_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		for (const char * const *k = invalid_keys; *k; k++) {
			const char *invalid_key = *k;

			int status = config_data_delete(config, invalid_key);
			assert_eq_msg(-1, status, "config_data_delete should return -1 when key is invalid");
		}
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_delete_no_create_paths_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		int status = config_data_delete(config, "this.key.does.not.exist");
		assert_eq_msg(1, status, "config_data_delete should return 1 when key is missing");
		assert_eq_msg(0, config->subsections_len, "config_data_delete should not create additional subsections");
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_delete_fail_section_missing_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		int status = config_data_delete(config, "this.key.does.not.exist");
		assert_eq_msg(1, status, "config_data_delete should return 1 when key is missing");
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_delete_fail_property_missing_test)
{
	struct config_data *config;

	const char *key = "this.key.will.exist";
	const char *key1 = "this.key.will.exist1";

	TEST_START() {
		config_data_init(&config);

		int status = config_data_insert(config, key, "my value");
		assert_zero_msg(status, "config_data_insert should create property for key '%s'", key);

		status = config_data_delete(config, key1);
		assert_eq_msg(1, status, "config_data_delete should return 1 for key '%s'", key1);
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_delete_valid_key_pass_test)
{
	struct config_data *config;

	const char *key = "this.key.will.exist";

	TEST_START() {
		config_data_init(&config);

		int status = config_data_insert(config, key, "my value");
		assert_zero_msg(status, "config_data_insert should create property for key '%s'", key);

		const char *property = config_data_find(config, key);
		assert_nonnull_msg(property, "config property not found unexpectedly");

		status = config_data_delete(config, key);
		assert_eq_msg(0, status, "config_data_delete should return 0 for key '%s'", key);

		property = config_data_find(config, key);
		assert_null_msg(property, "config property not deleted");
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_find_no_create_paths_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		const char *property = config_data_find(config, "this.key.does.not.exit");
		assert_null_msg(property, "config property should not exist");

		assert_zero_msg(config->subsections_len, "config_data_find should not create sections");
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_find_not_found_return_null_test)
{
	struct config_data *config;

	TEST_START() {
		config_data_init(&config);

		const char *property = config_data_find(config, "this.key.does.not.exit");
		assert_null_msg(property, "config property should not exist");
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_find_found_return_value_test)
{
	struct config_data *config;

	const char *key = "this.key.will.exit";

	TEST_START() {
		config_data_init(&config);

		int status = config_data_insert(config, key, "my value");
		assert_zero_msg(status, "config_data_insert should create property for key '%s'", key);

		const char *property = config_data_find(config, key);
		assert_nonnull_msg(property, "config property not found unexpectedly");
	}

	config_data_release(&config);

	TEST_END();
}

TEST_DEFINE(config_data_get_section_key_test)
{
	struct config_data *config;
	struct strbuf key_buff;

	const char *keys[][2] = {
		{ "this", "" },
		{ "this.key", "this" },
		{ "this.\"key\".is", "this.key" },
		{ "this.key.\"is-new\".new", "this.key.\"is-new\"" },
		{ "my.key.is.new", "my.key.is" },
		{ NULL, NULL }
	};

	TEST_START() {
		config_data_init(&config);
		strbuf_init(&key_buff);

		for (size_t i = 0; ; i++) {
			const char *key = keys[i][0];
			if (!key)
				break;

			int status = config_data_insert(config, key, key);
			assert_zero_msg(status, "failed to insert into config for key '%s'", key);
		}

		assert_eq_msg(2, config->subsections_len, "unexpected number of subsections");

		// reconstruct key for root node should be empty
		int status = config_data_get_section_key(config, &key_buff);
		assert_zero_msg(status, "config_data_get_section_key failed to reconstruct section key for root node");
		assert_eq_msg(0, key_buff.len, "config_data_get_section_key should produce empty string for root node");

		struct config_data *current = config;
		for (size_t depth = 1; current; depth++) {
			assert_nonzero_msg(current->subsections_len, "no subsections for node at depth %lu", depth);
			current = current->subsections[0];

			const char *expected_section_key = keys[depth][1];

			strbuf_clear(&key_buff);
			status = config_data_get_section_key(current, &key_buff);
			assert_zero_msg(status, "config_data_get_section_key failed to reconstruct section key at depth %lu", depth);
			assert_string_eq_msg(expected_section_key, key_buff.buff, "config_data_get_section_key did not produce expected key '%s' for depth %lu", expected_section_key, depth);

			if (!current->subsections_len)
				current = NULL;
		}
	}

	strbuf_release(&key_buff);
	config_data_release(&config);

	TEST_END();
}

int config_data_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "config_data_init should initialize with expected defaults", config_data_init_test },
			{ "attempting to insert value with invalid key should fail", config_data_insert_invalid_key_test },
			{ "insertion should fail if duplicate already exists", config_data_insert_duplicate_fail_test },
			{ "insertion should succeed with valid keys", config_data_insert_valid_key_pass_test },
			{ "attempting to update value with invalid key should fail", config_data_update_invalid_key_fail_test },
			{ "new config paths should not be created when update fails because key missing", config_data_update_no_create_paths_test },
			{ "update should fail if section does not exist already", config_data_update_missing_section_fails_test },
			{ "update should fail if config property does not exist already", config_data_update_missing_property_fails_test },
			{ "update should succeed with valid keys", config_data_update_valid_key_pass_test },
			{ "attempting to delete value with invalid key should fail", config_data_delete_invalid_key_fail_test },
			{ "new config paths should not be created when deleting config value", config_data_delete_no_create_paths_test },
			{ "delete should fail if section does not exist already", config_data_delete_fail_section_missing_test },
			{ "delete should fail if config property does not exist already", config_data_delete_fail_property_missing_test },
			{ "delete should succeed with valid keys", config_data_delete_valid_key_pass_test },
			{ "searching for config property should not create sections", config_data_find_no_create_paths_test },
			{ "searching for config property should return NULL if not found", config_data_find_not_found_return_null_test },
			{ "searching for config property should return pointer to property value if found", config_data_find_found_return_value_test },
			{ "config_data_get_section_key should correctly reconstruct section keys from arbitrary nodes", config_data_get_section_key_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
