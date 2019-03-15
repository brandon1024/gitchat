#include "test-lib.h"
#include "parse-config.h"

TEST_DEFINE(parse_config_valid)
{
	struct conf_data cd;

	TEST_START() {
		char *conf_path = "resources/good.config";
		struct conf_data_entry *entry;
		int ret = parse_config(&cd, conf_path);
		assert_zero_msg(ret, "parse_config() should return zero on good config.");

		/* ensure section names are valid */
		assert_eq_msg(4, cd.sections.len, "Expected 4 sections in config, but got %zu.", cd.sections.len);
		assert_string_eq("user", cd.sections.strings[0]);
		assert_string_eq("core", cd.sections.strings[1]);
		assert_string_eq("sendemail", cd.sections.strings[2]);
		assert_string_eq("alias.alias", cd.sections.strings[3]);
		assert_eq_msg(37, cd.entries_len, "Expected 37 entries, but got %zu.", cd.entries_len);

		/* ensure key-value parsed correctly with no section */
		entry = cd.entries[9];
		assert_nonnull(entry->section);
		assert_string_eq("sendemail", entry->section);
		assert_eq_msg(cd.sections.strings[2], entry->section, "The pointer to the section name in the entry should point to the correct string.");
		assert_string_eq("smtpencryption", entry->key);
		assert_string_eq("tls", entry->value);

		/* ensure key-value parsed correctly with varying whitespace formats */
		entry = cd.entries[10];
		assert_nonnull(entry->section);
		assert_string_eq("sendemail", entry->section);
		assert_eq_msg(cd.sections.strings[2], entry->section, "The pointer to the section name in the entry should point to the correct string.");
		assert_string_eq("smtpserver", entry->key);
		assert_string_eq("smtp.gmail.com", entry->value);

		entry = cd.entries[11];
		assert_nonnull(entry->section);
		assert_string_eq("sendemail", entry->section);
		assert_eq_msg(cd.sections.strings[2], entry->section, "The pointer to the section name in the entry should point to the correct string.");
		assert_string_eq("smtpuser", entry->key);
		assert_string_eq("\"brandon1024.br@gmail.com\"", entry->value);

		entry = cd.entries[12];
		assert_nonnull(entry->section);
		assert_string_eq("sendemail", entry->section);
		assert_eq_msg(cd.sections.strings[2], entry->section, "The pointer to the section name in the entry should point to the correct string.");
		assert_string_eq("smtppass", entry->key);
		assert_string_eq("password", entry->value);

		/* ensure long values parses correctly */
		entry = cd.entries[28];
		assert_nonnull(entry->section);
		assert_string_eq("alias.alias", entry->section);
		assert_eq_msg(cd.sections.strings[3], entry->section, "The pointer to the section name in the entry should point to the correct string.");
		assert_string_eq("bare", entry->key);
		assert_string_eq("!sh -c 'git symbolic-ref HEAD refs/heads/$1 && git rm --cached -r . && git clean -xfd' -", entry->value);
	}

	release_config_resources(&cd);
	TEST_END();
}

TEST_DEFINE(parse_config_invalid_conf_path)
{
	struct conf_data cd;

	TEST_START() {
		char *config_path = "resources/conf_does_not_exit.config";
		int ret = parse_config(&cd, config_path);
		assert_nonzero_msg(ret, "parse_config() should return non-zero when config file does not exist.");
	}

	release_config_resources(&cd);
	TEST_END();
}

TEST_DEFINE(parse_config_invalid_section)
{
	struct conf_data cd;

	TEST_START() {
		char *config_path = "resources/bad_section_1.config";
		int ret = parse_config(&cd, config_path);
		assert_nonzero_msg(ret, "parse_config() should return non-zero with config file with invalid section syntax.");

		config_path = "resources/bad_section_2.config";
		ret = parse_config(&cd, config_path);
		assert_nonzero_msg(ret, "parse_config() should return non-zero with config file with invalid section syntax.");

		config_path = "resources/bad_section_3.config";
		ret = parse_config(&cd, config_path);
		assert_nonzero_msg(ret, "parse_config() should return non-zero with config file with invalid section syntax.");
	}

	release_config_resources(&cd);
	TEST_END();
}

TEST_DEFINE(parse_config_invalid_key_value)
{
	struct conf_data cd;

	TEST_START() {
		char *config_path = "resources/bad_key_1.config";
		int ret = parse_config(&cd, config_path);
		assert_nonzero_msg(ret, "parse_config() should return non-zero with config file with invalid key syntax.");

		config_path = "resources/bad_key_2.config";
		ret = parse_config(&cd, config_path);
		assert_nonzero_msg(ret, "parse_config() should return non-zero with config file with invalid key syntax.");

		config_path = "resources/bad_key_3.config";
		ret = parse_config(&cd, config_path);
		assert_nonzero_msg(ret, "parse_config() should return non-zero with config file with invalid key syntax.");

		config_path = "resources/bad_key_4.config";
		ret = parse_config(&cd, config_path);
		assert_nonzero_msg(ret, "parse_config() should return non-zero with config file with invalid key syntax.");

		config_path = "resources/bad_key_5.config";
		ret = parse_config(&cd, config_path);
		assert_nonzero_msg(ret, "parse_config() should return non-zero with config file with invalid key syntax.");
	}

	release_config_resources(&cd);
	TEST_END();
}

int parse_config_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "parse-config should correctly parse a valid config", parse_config_valid },
			{ "parse-config should fail with invalid config file path", parse_config_invalid_conf_path },
			{ "parse-config should fail with invalid section names", parse_config_invalid_section },
			{ "parse-config should fail with invalid key-value pairs", parse_config_invalid_key_value },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}