#ifndef GIT_CHAT_PARSE_CONFIG_H
#define GIT_CHAT_PARSE_CONFIG_H

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
 * Key-value pairs can be defined outside of a section:
 * ```
 * key_3 = valuec
 *
 * [ section_a ]
 *     key_1 = value
 *     key_2 = valueb
 * ```
 *
 * Section names, key names, and values can take one of two forms; quoted and
 * unquoted.
 *
 * In the unquoted form, sections, keys and values can only contain the
 * characters [a-zA-Z0-9_].
 *
 * The quoted form can be used to name the section, key, or value with
 * a special character (any printable ascii character, other than double quote
 * '"'). For instance:
 * ```
 * [ "section-a" ]
 *     "key.1" = value
 *     key_2 = "this is a value with spaces"
 * ```
 *
 *
 * Configuration File Value Addressing:
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
 *
 * When sections or keys are quoted, they can be addressed in the following
 * manner:
 * "section with symbol"."key.with.symbol"
 *
 * Although sections cannot be nested, section names can have a dot ".", to
 * indicate that the section is represented as a subsection:
 * ```
 * [ section ]
 *     key_1 = value
 * [ section.subsection ]
 *     key_2 = value
 * ```
 * */

#define CONF_DATA_INIT() {NULL, NULL, 0}

struct conf_data {
	char **keys;
	char **values;
	size_t len;
};

/**
 * Parse the config file, and represent its contents in struct conf_data. This
 * function may be used when multiple queries into the config file are necessary,
 * and rather than sequentially parse the file for every key, like
 * parse_config_callback() does, simply parse the file once.
 *
 * Returns non-zero if the file could not be parsed, and 0 if successful.
 * */
int parse_config(const char *conf_path, struct conf_data *conf);

/**
 * Query a struct conf_data for a key, returning the value if found, or NULL if
 * does not exist.
 * */
char *find_value(struct conf_data *conf, const char *key);

/**
 * Release any resources under a struct conf_data.
 * */
void release_config_resources(struct conf_data *conf);

/**
 * Search the config file sequentially until the specified key is found, and
 * invoke the callback function with the value.
 *
 * The callback function must have the following signature:
 * void callback(const char *key, const char *value, void *data);
 *
 * If the key cannot be found, the callback function is invoked with NULL
 * `value`.
 *
 * This function returns non-zero if the key was not found, and 0 if the key
 * exists.
 * */
int parse_config_callback(const char *conf_path, const char *key, void *data,
		void (*callback)(const char *key, const char *value, void *data));

#endif //GIT_CHAT_PARSE_CONFIG_H
