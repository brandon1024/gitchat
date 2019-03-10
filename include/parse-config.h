#ifndef GIT_CHAT_PARSE_CONFIG_H
#define GIT_CHAT_PARSE_CONFIG_H

#include "str-array.h"

/**
 * Parse-Config API
 *
 * Configuration File Format:
 * The config file has a format similar to INI or TOML, but simplified. It
 * consists of key-value pairs, organized in sections. For example:
 *
 * ```
 * [ section_a ]
 *     key_1 = value
 *     key_2 = valueb
 *
 * [ section_b ]
 *     key_1 = 123
 *     key_a = 321
 * ```
 *
 * Whitespace Rules:
 * Leading and trailing whitespace is ignored. So,
  ```
 * [ section_a ]
 *     key_1 = value
 *     key_2 = valueb
 * ```
 *
 * is equivalent to
 * ```
 * [ section_b ]
 * key_1 = 123
 * key_a = 321
 * ```
 *
 * Furthermore, whitespace between the key name and `=`, and between `=` and the
 * value is ignored. So, these statements are equivalent:
 * ```
 * key_1 = 123
 * key_1=	123
 * key_1=123
 * ```
 *
 * Trailing whitespace are values are also ignored. However, values can be quoted
 * with double quotes to preserve whitespace.
 *
 *
 * Sections:
 * Key-value pairs can be defined outside of a section, but must appear before
 * any section declaration:
 * ```
 * key_3 = valuec
 *
 * [ section_a ]
 *     key_1 = value
 *     key_2 = valueb
 * ```
 *
 * Although sections cannot be nested, section names can have a period `.`,
 * which can be used to help indicate that the section is be represented as a
 * subsection:
 * ```
 * [ section ]
 *     key_1 = value
 * [ section.subsection ]
 *     key_2 = value
 * ```
 *
 * Section and key names may only contain alphanumeric characters, underscores
 * `_`, and periods `.`. Use of disallowed characters may result in undesired or
 * undefined behavior.
 *
 * Duplicate section names are allowed, but duplicate addresses will result in
 * undefined behavior. For instance, the following is disallowed because the
 * address `section_a.key_1` is defined twice:
 * ```
 * [ section_a ]
 *     key_1 = value
 *     key_2 = valueb
 *
 * [ section_a ]
 *     key_1 = 123
 *     key_a = 321
 * ```
 *
 * Addressing Key-Value Pairs:
 * Values in the config file are addressed using the dot ".", as shown below:
 * ```
 * [ section_a ]
 *     key_1 = value
 *     key_2 = valueb
 *
 * [ section_b ]
 *     key_a = 123
 *     key_b = 321
 * ```
 *
 * section_a.key_1
 * section_b.key_a
 * */

struct conf_data_entry {
	char *section;
	char *key;
	char *value;
};

struct conf_data {
	struct str_array sections;
	struct conf_data_entry **entries;
	size_t entries_len;
	size_t entries_alloc;
};

/**
 * Parse the config file, and represent its contents in struct conf_data. This
 * function may be used when multiple queries into the config file are necessary,
 * and rather than sequentially parse the file for every key, like
 * parse_config_callback() does, simply parse the file once.
 *
 * Returns:
 * - 0 if the file was parsed successfully
 * - <0 if the file could not be read (file not found, insufficient permissions)
 * - >0 if the file could not be parsed due to a syntax error
 * */
int parse_config(struct conf_data *conf, const char *conf_path);

/**
 * Query a struct conf_data for a key, returning the value if found, or NULL if
 * does not exist.
 * */
char *find_value(struct conf_data *conf, char *key);

/**
 * Release any resources under a struct conf_data.
 * */
void release_config_resources(struct conf_data *conf);

#endif //GIT_CHAT_PARSE_CONFIG_H
