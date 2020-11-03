#ifndef GIT_CHAT_INCLUDE_CONFIG_CONFIG_KEY_H
#define GIT_CHAT_INCLUDE_CONFIG_CONFIG_KEY_H

#include "str-array.h"
#include "strbuf.h"

/**
 * Check that a given key string is valid, returning zero of valid and non-zero
 * if invalid.
 *
 * A valid key must meet the following criteria:
 * - must be printable characters
 * - unquoted, must be comprised of alphanumeric characters, including '.' and '_',
 * - have sections delimited by periods '.',
 * - sections may contain other symbols if the section component is surrounded in double quotes,
 * - within a quoted section component, double quotes and slashes can be escaped as '\"' and '\\'
 * */
int is_valid_config_key(const char *key);

/**
 * Split a config key into normalized key components, returning zero if the key
 * is valid and split successfully and non-zero if the key could not be split
 * into components.
 *
 * If a key component is quoted, the quotes are removed. Escaped characters are
 * unescaped.
 * */
int isolate_config_key_components(const char *key, struct str_array *components);

/**
 * Merge a list of normalized key components into a single key. This is
 * effectively the reverse operation of isolate_config_key_components().
 *
 * Slashes and double quotes are escaped if encountered.
 *
 * Returns zero if successful, and non-zero if one or more key components are
 * invalid.
 * */
int merge_config_key_components(struct str_array *components, struct strbuf *key);

/**
 * If `buff` contains any non-alphanumeric characters (exception for '_'),
 * the buffer is wrapped in double quotes and any quotes or backslashes are escaped.
 * */
void escape_buffer(struct strbuf *buff);

/**
 * Remove escape sequences from a given buffer.
 *
 * The following standard escape sequences are recognized:
 * \', \", \?, \\
 *
 * Returns 0 if successful and non-zero if the buffer is not formatted correctly.
 * */
void unescape_buffer(struct strbuf *buffer);

#endif //GIT_CHAT_INCLUDE_CONFIG_CONFIG_KEY_H
