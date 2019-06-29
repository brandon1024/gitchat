#ifndef GIT_CHAT_PARSE_CONFIG_H
#define GIT_CHAT_PARSE_CONFIG_H

/**
 * parse-config api
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
 * Trailing whitespace are values are also ignored.
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
 * Key-Value Pairs:
 * Key-value pairs take the form `<key> = <value>`. Any characters between the
 * first `=` and end of line are parsed as the value, with the exception that
 * leading and trailing whitespace is trimmed. Values may not spread over
 * multiple lines.
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

#include <stdlib.h>
#include "str-array.h"

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
 * Parse the config file, and represent its contents in struct conf_data.
 *
 * Returns:
 * - 0 if the file was parsed successfully
 * - <0 if the file could not be read (file not found, insufficient permissions)
 * - >0 if the file could not be parsed due to a syntax error
 * */
int parse_config(struct conf_data *conf, const char *conf_path);

/**
 * Write a struct conf_data to a file.
 *
 * If a file at the path 'conf_path' already exists, it is overwritten.
 * */
void write_config(struct conf_data *conf, const char *conf_path);

/**
 * Query a struct conf_data with a given secion and key, returning a pointer to
 * to struct conf_data_entry if found, or NULL if does not exist.
 * */
struct conf_data_entry *conf_data_find_entry(struct conf_data *conf,
		char *section, char *key);

/**
 * Sort a struct conf_data in strcmp() order, first sorting sections then sorting
 * keys.
 * */
void conf_data_sort(struct conf_data *conf);

/**
 * Release any resources under a struct conf_data.
 * */
void release_config_resources(struct conf_data *conf);

#endif //GIT_CHAT_PARSE_CONFIG_H
