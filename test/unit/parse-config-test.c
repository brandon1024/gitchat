#include "test-lib.h"
#include "parse-config.h"

TEST_DEFINE(parse_config_valid)
{
	struct conf_data cd;

	TEST_START() {
		char *conf_path = "resources/good_1.config";
		struct conf_data_entry *entry;
		int ret = parse_config(&cd, conf_path);
		assert_zero_msg(ret, "parse_config() should return zero on good config.");

		/* ensure section names are valid */
		assert_eq_msg(4, cd.sections.len, "Expected 4 sections in config, but got %zu.", cd.sections.len);
		assert_string_eq("user", cd.sections.entries[0].string);
		assert_string_eq("core", cd.sections.entries[1].string);
		assert_string_eq("sendemail", cd.sections.entries[2].string);
		assert_string_eq("alias.alias", cd.sections.entries[3].string);
		assert_eq_msg(38, cd.entries_len, "Expected 37 entries, but got %zu.", cd.entries_len);

		/* ensure key-value parsed correctly with no section */
		entry = cd.entries[10];
		assert_nonnull(entry->section);
		assert_string_eq("sendemail", entry->section);
		assert_eq_msg(cd.sections.entries[2].string, entry->section, "The pointer to the section name in the entry should point to the correct string.");
		assert_string_eq("smtpencryption", entry->key);
		assert_string_eq("", entry->value);

		/* ensure key-value parsed correctly with varying whitespace formats */
		entry = cd.entries[11];
		assert_nonnull(entry->section);
		assert_string_eq("sendemail", entry->section);
		assert_eq_msg(cd.sections.entries[2].string, entry->section, "The pointer to the section name in the entry should point to the correct string.");
		assert_string_eq("smtpserver", entry->key);
		assert_string_eq("smtp.gmail.com", entry->value);

		entry = cd.entries[12];
		assert_nonnull(entry->section);
		assert_string_eq("sendemail", entry->section);
		assert_eq_msg(cd.sections.entries[2].string, entry->section, "The pointer to the section name in the entry should point to the correct string.");
		assert_string_eq("smtpuser", entry->key);
		assert_string_eq("brandon1024.br@gmail.com", entry->value);

		entry = cd.entries[13];
		assert_nonnull(entry->section);
		assert_string_eq("sendemail", entry->section);
		assert_eq_msg(cd.sections.entries[2].string, entry->section, "The pointer to the section name in the entry should point to the correct string.");
		assert_string_eq("smtppass", entry->key);
		assert_string_eq("password", entry->value);

		/* ensure long values parses correctly */
		entry = cd.entries[29];
		assert_nonnull(entry->section);
		assert_string_eq("alias.alias", entry->section);
		assert_eq_msg(cd.sections.entries[3].string, entry->section, "The pointer to the section name in the entry should point to the correct string.");
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

TEST_DEFINE(conf_data_sort_test)
{
	struct conf_data cd;

	TEST_START() {
		char *config_path = "resources/good_2.config";
		int ret = parse_config(&cd, config_path);
		assert_zero_msg(ret, "parse_config() should return zero with valid config.");
		assert_eq_msg(6, cd.entries_len, "parse_config() returned incorrect number of entries.");

		//check order before sort
		assert_string_eq("user", cd.entries[0]->section);
		assert_string_eq("name", cd.entries[0]->key);
		assert_string_eq("user", cd.entries[1]->section);
		assert_string_eq("email", cd.entries[1]->key);
		assert_string_eq("user", cd.entries[2]->section);
		assert_string_eq("username", cd.entries[2]->key);
		assert_string_eq("core", cd.entries[3]->section);
		assert_string_eq("editor", cd.entries[3]->key);
		assert_string_eq("core", cd.entries[4]->section);
		assert_string_eq("whitespace", cd.entries[4]->key);
		assert_string_eq("core", cd.entries[5]->section);
		assert_string_eq("excludesfile", cd.entries[5]->key);

		conf_data_sort(&cd);

		//check order after sort
		assert_string_eq("core", cd.entries[0]->section);
		assert_string_eq("editor", cd.entries[0]->key);
		assert_string_eq("core", cd.entries[1]->section);
		assert_string_eq("excludesfile", cd.entries[1]->key);
		assert_string_eq("core", cd.entries[2]->section);
		assert_string_eq("whitespace", cd.entries[2]->key);

		assert_string_eq("user", cd.entries[3]->section);
		assert_string_eq("email", cd.entries[3]->key);
		assert_string_eq("user", cd.entries[4]->section);
		assert_string_eq("name", cd.entries[4]->key);
		assert_string_eq("user", cd.entries[5]->section);
		assert_string_eq("username", cd.entries[5]->key);
	}

	release_config_resources(&cd);

	TEST_END();
}

TEST_DEFINE(conf_data_find_entry_test) {
	struct conf_data cd;
	struct conf_data_entry *entry;

	TEST_START() {
		char *config_path = "resources/good_1.config";
		int ret = parse_config(&cd, config_path);
		assert_zero_msg(ret, "parse_config() should return zero with valid config.");

		//without section
		entry = conf_data_find_entry(&cd, NULL, "remote");
		assert_nonnull_msg(entry, "conf_data_find_entry() did not find an entry with key 'remote'");
		assert_null(entry->section);
		assert_string_eq("remote", entry->key);
		assert_string_eq("", entry->value);

		//with section
		entry = conf_data_find_entry(&cd, "core", "editor");
		assert_nonnull_msg(entry, "conf_data_find_entry() did not find an entry with section 'core' and key 'editor'");
		assert_string_eq("core", entry->section);
		assert_string_eq("editor", entry->key);
		assert_string_eq("vim", entry->value);

		//inexistent entry
		assert_null(conf_data_find_entry(&cd, "core", "hello"));
		assert_null(conf_data_find_entry(&cd, NULL, "hello"));
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
			{ "conf_data_sort should correctly sort the conf_data", conf_data_sort_test },
			{ "conf_data_find_entry should correctly find the entry with the given section and key", conf_data_find_entry_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
