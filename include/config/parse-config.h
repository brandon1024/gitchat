#ifndef GIT_CHAT_INCLUDE_CONFIG_PARSE_CONFIG_H
#define GIT_CHAT_INCLUDE_CONFIG_PARSE_CONFIG_H

#include "config/config-data.h"
#include "config/config-key.h"
#include "config/config-defaults.h"

/**
 * parse-config api
 *
 * Configuration File Format:
 * -------------------------
 * The config file has a format similar to INI, TOML or Git's own config file
 * format, albeit simplified. It consists of key-value pairs, organized in
 * sections. For example:
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
 * -------------------------
 * Leading and trailing whitespace is generally ignored, unless escaped.
 * Whitespace surrounding section names, property names and property values
 * are removed, so
  ```
 * [ section_a ]
 *     key_1 = value
 *     key_2 = valueb
 * ```
 *
 * is equivalent to
 * ```
 * [section_b]
 * key_1 = 123
 * key_a = 321
 * ```
 *
 * Section names, property names and property values cannot span multiple lines.
 *
 * Sections and Subsections:
 * -------------------------
 * Key-value pairs can be defined outside of a section, but must appear before
 * any section declaration, or after an empty section:
 * ```
 * key_3 = valuec
 *
 * [ section_a ]
 *     key_1 = value
 *     key_2 = valueb
 * []
 *     key_4 = valued
 * ```
 *
 * Sections cannot be nested, but section names can can be period-delimited,
 * which can be used to help indicate that the section is be represented as a
 * subsection:
 * ```
 * [ section ]
 *     key_1 = value
 * [ section.subsection ]
 *     key_2 = value
 * ```
 *
 * Duplicate section names and properties are allowed. Duplicate properties will
 * simply override earlier declarations.
 * ```
 * [ section_a ]
 *     key_1 = value
 *     key_2 = valueb
 * [ section_a ]
 *     key_1 = 123
 *     key_a = 321
 * ```
 *
 * In the example above, the following effective properties are defined:
 * ```
 * section_a.key_1 = 123
 * section_a.key_2 = valueb
 * section_a.key_a = 321
 * ```
 *
 * Section names are case sensitive.
 *
 * Key-Value Pairs:
 * -------------------------
 * Key-value pairs take the form `<key> = <value>`. Any characters between the
 * first `=` and end of line are parsed as the value, with the exception that
 * leading and trailing whitespace is trimmed. Values may not spread over
 * multiple lines.
 *
 * Property names are case sensitive.
 *
 * Property names may contain period-delimited components. For example, the
 * following config file is valid:
 *
 * ```
 * section.subsection.property1 = value1
 * section.subsection.property2 = value2
 * ```
 *
 * Property names and values may be surrounded in single or double quotes
 * (see further).
 *
 * Addressing Key-Value Pairs:
 * -------------------------
 * Values in a config file are addressed by the section they belong to, and the
 * property name. Section names and properties are delimited by a period `.`:
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
 * Characters, Quotes and Escaping:
 * -------------------------
 * Unquoted, section names and property names can only contain alphanumeric
 * characters and underscores (`_`).
 *
 * When a section name or property name must contain symbols, the section
 * name or property name may be surrounded in double quotes. For example:
 *
 * ```
 * [section.with."$ymbols"]
 *     "pr()perty" = value
 * ```
 *
 * Double quotes (`"`) and backslashes (`\`) can be escaped as `\"` and `\\`,
 * respectively. Backslashes preceding other characters are simply removed, so
 * `\n` becomes `n`.
 * */

/**
 * Attempt to parse a config file from the given path on the file system.
 *
 * `conf` must be initialized.
 *
 * Returns:
 * - 0 if the file was parsed successfully
 * - <0 if the file could not be opened for reading
 * - >0 if the file could not be parsed due to a syntax error
 * */
int parse_config(struct config_data *conf, const char *conf_path);

/**
 * Attempt to parse a config file from the given file descriptor.
 *
 * `conf` must be initialized.
 *
 * Returns:
 * - 0 if the file was parsed successfully
 * - <0 if the file could not be opened for reading
 * - >0 if the file could not be parsed due to a syntax error
 * */
int parse_config_fd(struct config_data *conf, int fd);

/**
 * Serialize config data to a file with the given path. The contents of the file
 * at the given path are overridden if the file exists.
 *
 * Returns:
 * - 0 if the config was written successfully
 * - < 0 if unable to open the destination file for writing
 * - > 0 non-zero if the config failed validation.
 * */
int write_config(struct config_data *conf, const char *conf_path);

/**
 * Serialize config data by writing the data to the given file descriptor.
 *
 * Returns:
 * - 0 if the config was written successfully
 * - > 0 non-zero if the config failed validation.
 * */
int write_config_fd(struct config_data *conf, int fd);

/**
 * Check whether a file at the given path is a valid configuration file that
 * can be parsed by the application, optionally checking that all the keys are
 * recognized by the application.
 *
 * If the config file cannot be parsed, returns -1. If the config file is
 * invalid, returns 1. Otherwise returns zero.
 *
 * If recognized_keys_only is non-zero, returns 2 if the config file contains
 * unrecognized keys.
 * */
int is_config_invalid(const char *conf_path, int recognized_keys_only);


#endif //GIT_CHAT_INCLUDE_CONFIG_PARSE_CONFIG_H
