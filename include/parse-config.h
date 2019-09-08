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

struct config_entry;
struct config_file_data {
	struct config_entry *head;
	struct config_entry *tail;
};

/**
 * Initialize a config_file_data structure for use.
 * */
void config_file_data_init(struct config_file_data *conf);

/**
 * Parse the config file, and represent its contents in struct config_file_data.
 *
 * Note that this function will initialize the config_file_data structure before
 * use.
 *
 * `conf_path` must be a null-terminated string.
 *
 * Returns:
 * - 0 if the file was parsed successfully
 * - <0 if the file could not be read (file not found, insufficient permissions)
 * - >0 if the file could not be parsed due to a syntax error
 * */
int parse_config(struct config_file_data *conf, const char *conf_path);

/**
 * Write a struct config_file_data to a file. `conf_path` must be a
 * null-terminated string. If a file at the path 'conf_path' already exists, it
 * is overwritten.
 *
 * Returns:
 * - 0 if the config was written successfully
 * - < 0 if unable to open the destination file for writing
 * - > 0 non-zero if the config failed validation.
 * */
int write_config(struct config_file_data *conf, const char *conf_path);

/**
 * Release any resources under a config_file_data structure.
 * */
void config_file_data_release(struct config_file_data *conf);

/**
 * Insert a new config entry into the config_file_data, returning the new entry or NULL
 * if the key is invalid.
 * */
struct config_entry *config_file_data_insert_entry(struct config_file_data *conf, const char *key, const char *value);

/**
 * Delete a config entry from the config_file_data. All resources under the entry are released.
 * */
void config_file_data_delete_entry(struct config_file_data *conf, struct config_entry *entry);

/**
 * Attempt to find a config entry with a given key. If no such entry exists,
 * returns NULL.
 * */
struct config_entry *config_file_data_find_entry(struct config_file_data *conf, const char *key);

/**
 * Get the value for a config entry. The string returned should not be mutated.
 * */
char *config_file_data_get_entry_value(struct config_entry *entry);

/**
 * Set the value for a config entry. The previous entry value is free()d, and the
 * new value is duplicated.
 * */
void config_file_data_set_entry_value(struct config_entry *entry, const char *value);

/**
 * Get the key for a config entry. The string returned should not be mutated.
 * */
char *config_file_data_get_entry_key(struct config_entry *entry);

#endif //GIT_CHAT_PARSE_CONFIG_H
