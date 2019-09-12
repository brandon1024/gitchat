#include "test-lib.h"
#include "parse-config.h"
#include "config-defaults.h"

TEST_DEFINE(initialize_config_file_data_struct)
{
	struct config_file_data cd;

	TEST_START() {
		config_file_data_init(&cd);
		assert_null_msg(cd.head, "empty config_file_data should have a NULL head");
		assert_null_msg(cd.tail, "empty config_file_data should have a NULL tail");
	}

	TEST_END();
}

TEST_DEFINE(parse_config_valid)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/good_1.config";
		struct config_entry *entry = NULL;
		int ret = parse_config(&cd, conf_path);
		assert_zero_msg(ret, "parse_config() should return zero on good config.");

		const char *key = "current";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq("yellow bold", config_file_data_get_entry_value(entry));

		key = "remote";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq("", config_file_data_get_entry_value(entry));

		key = "hi";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq("", config_file_data_get_entry_value(entry));

		key = "sendemail.smtpencryption";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq("", config_file_data_get_entry_value(entry));

		key = "alias.alias.theirs";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq("!f() { git checkout --theirs $@ && git add $@; }; f", config_file_data_get_entry_value(entry));
	}

	config_file_data_release(&cd);
	TEST_END();
}

TEST_DEFINE(insert_entry_into_config_data)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/good_1.config";
		struct config_entry *entry = NULL;
		int ret = parse_config(&cd, conf_path);
		assert_zero_msg(ret, "parse_config() should return zero on good config.");

		const char *key = "my.new.key";
		entry = config_file_data_find_entry(&cd, key);
		assert_null_msg(entry, "found unexpected entry with key '%s'", key);

		const char *value = "my value!";
		config_file_data_insert_entry(&cd, key, value);
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "entry with key '%s' was not inserted", key);
		assert_string_eq(value, config_file_data_get_entry_value(entry));
	}

	config_file_data_release(&cd);
	TEST_END();
}

TEST_DEFINE(delete_entry_from_config_data)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/good_1.config";
		struct config_entry *entry = NULL;
		int ret = parse_config(&cd, conf_path);
		assert_zero_msg(ret, "parse_config() should return zero on good config.");

		const char *key = "sendemail.smtpuser";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq("brandon1024.br@gmail.com", config_file_data_get_entry_value(entry));

		config_file_data_delete_entry(&cd, entry);
		entry = config_file_data_find_entry(&cd, key);
		assert_null_msg(entry, "entry with key '%s' was not deleted", key);
	}

	config_file_data_release(&cd);
	TEST_END();
}

TEST_DEFINE(find_entry_in_config_data)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/good_1.config";
		struct config_entry *entry = NULL;
		int ret = parse_config(&cd, conf_path);
		assert_zero_msg(ret, "parse_config() should return zero on good config.");

		const char *key = "sendemail.smtpserver";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq("smtp.gmail.com", config_file_data_get_entry_value(entry));

		key = "unknown.key";
		entry = config_file_data_find_entry(&cd, key);
		assert_null_msg(entry, "entry with key '%s' should not exist", key);
	}

	config_file_data_release(&cd);
	TEST_END();
}

TEST_DEFINE(set_config_entry_value)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/good_1.config";
		struct config_entry *entry = NULL;
		int ret = parse_config(&cd, conf_path);
		assert_zero_msg(ret, "parse_config() should return zero on good config.");

		const char *key = "sendemail.smtpserver";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq("smtp.gmail.com", config_file_data_get_entry_value(entry));

		const char *value = "my new vlaue";
		config_file_data_set_entry_value(entry, value);
		assert_string_eq(value, config_file_data_get_entry_value(entry));
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(get_config_entry_key)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/good_2.config";
		struct config_entry *entry = NULL;
		int ret = parse_config(&cd, conf_path);
		assert_zero_msg(ret, "parse_config() should return zero on good config.");

		const char *key = "core.whitespace";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq(key, config_file_data_get_entry_key(entry));
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(get_config_entry_value)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/good_2.config";
		struct config_entry *entry = NULL;
		int ret = parse_config(&cd, conf_path);
		assert_zero_msg(ret, "parse_config() should return zero on good config.");

		const char *key = "core.excludesfile";
		entry = config_file_data_find_entry(&cd, key);
		assert_nonnull_msg(entry, "could not find entry with key '%s'", key);
		assert_string_eq("~/.gitignore", config_file_data_get_entry_value(entry));
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(parse_config_invalid_empty_key)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/bad_key_1.config";
		int ret = parse_config(&cd, conf_path);
		assert_true_msg(ret > 0, "parse_config() should return positive status on invalid config (parse error) but was %d.", ret);
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(parse_config_invalid_key_character)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/bad_key_2.config";
		int ret = parse_config(&cd, conf_path);
		assert_true_msg(ret > 0, "parse_config() should return positive status on invalid config (parse error) but was %d.", ret);
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(parse_config_invalid_key_with_space)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/bad_key_3.config";
		int ret = parse_config(&cd, conf_path);
		assert_true_msg(ret > 0, "parse_config() should return positive status on invalid config (parse error) but was %d.", ret);
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(parse_config_invalid_line_no_eq_symbol)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/bad_key_4.config";
		int ret = parse_config(&cd, conf_path);
		assert_true_msg(ret > 0, "parse_config() should return positive status on invalid config (parse error) but was %d.", ret);
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(parse_config_invalid_section_trailing_characters)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/bad_section_1.config";
		int ret = parse_config(&cd, conf_path);
		assert_true_msg(ret > 0, "parse_config() should return positive status on invalid config (parse error) but was %d.", ret);
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(parse_config_invalid_section_character)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/bad_section_2.config";
		int ret = parse_config(&cd, conf_path);
		assert_true_msg(ret > 0, "parse_config() should return positive status on invalid config (parse error) but was %d.", ret);

		conf_path = "resources/bad_section_3.config";
		ret = parse_config(&cd, conf_path);
		assert_true_msg(ret > 0, "parse_config() should return positive status on invalid config (parse error) but was %d.", ret);
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(parse_config_read_error)
{
	struct config_file_data cd;

	TEST_START() {
		char *conf_path = "resources/unknown.config";
		int ret = parse_config(&cd, conf_path);
		assert_true_msg(ret < 0, "parse_config() should return negative status on read error but was %d.", ret);
	}

	config_file_data_release(&cd);

	TEST_END();
}

TEST_DEFINE(is_config_valid_test)
{
	TEST_START() {
		assert_eq(-1, is_config_invalid("resources/unknown.config", 0));
		assert_eq(-1, is_config_invalid("resources/unknown.config", 1));

		const char *invalid_configs[] = {
				"resources/bad_key_1.config",
				"resources/bad_key_2.config",
				"resources/bad_key_3.config",
				"resources/bad_key_4.config",
				"resources/bad_section_1.config",
				"resources/bad_section_2.config",
				"resources/bad_section_3.config",
				NULL
		};

		const char **conf = invalid_configs;
		while (*conf) {
			int is_invalid = is_config_invalid(*conf, 0);
			assert_eq(1, is_invalid);
			conf++;
		}

		const char *valid_configs[] = {
				"resources/good_1.config",
				"resources/good_2.config",
				"resources/good_3.config",
				NULL
		};

		conf = valid_configs;
		while (*conf) {
			assert_zero(is_config_invalid(*conf, 0));
			conf++;
		}

		assert_eq(2, is_config_invalid(valid_configs[0], 1));
		assert_eq(2, is_config_invalid(valid_configs[1], 1));
		assert_zero(is_config_invalid(valid_configs[2], 1));
	}

	TEST_END();
}

int parse_config_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "initializing empty config_file_data should have NULL entry head and tail", initialize_config_file_data_struct },
			{ "parse-config should correctly parse a valid config", parse_config_valid },
			{ "inserting entry into config data should insert correctly", insert_entry_into_config_data },
			{ "deleting entry from config data should no longer appear in list of entries", delete_entry_from_config_data },
			{ "searching for entries in a config_file_data should return expected entries", find_entry_in_config_data },
			{ "set config entry value should duplicate string and persist", set_config_entry_value },
			{ "get entry key from entry structure should return expected value", get_config_entry_key },
			{ "get entry value from entry structure should return expected value", get_config_entry_value },
			{ "parse_config with invalid empty key should return non-zero", parse_config_invalid_empty_key },
			{ "parse_config with invalid key character should return non-zero", parse_config_invalid_key_character },
			{ "parse_config with invalid space in key should return non-zero", parse_config_invalid_key_with_space },
			{ "parse_config with invalid line (no = symbol) should return non-zero", parse_config_invalid_line_no_eq_symbol },
			{ "parse_config with invalid section (trailing chars) should return non-zero", parse_config_invalid_section_trailing_characters },
			{ "parse_config with invalid section (invalid chars) should return non-zero", parse_config_invalid_section_character },
			{ "parse_config with unknown file (read error) should return non-zero", parse_config_read_error },
			{ "is_config_valid should correctly identify invalid config files", is_config_valid_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
