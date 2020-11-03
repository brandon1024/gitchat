#include <string.h>
#include <ctype.h>

#include "config/config-key.h"

/**
 * Check if a quoted key component is valid, returning zero if invalid and non-zero if
 * valid.
 *
 * This family of functions is intended to be executed for each component of a
 * key to determine whether a config key literal is valid. These functions may
 * also be used to extract the individual components of a key.
 *
 * `comp_start` represents the start of a key component, which is expected to
 * begin with a double quote. `tail` is updated with a pointer to the
 * end of the component, which may be a period or null terminator.
 * */
static int is_valid_quoted_key_component(const char *comp_start, const char **tail)
{
	const char *end;

	for (end = comp_start + 1; *end && *end != '"'; end++) {
		if (!isprint(*end))
			return 0;

		// if backslash, skip the next character (or return if end of string)
		if (*end == '\\')
			end++;
	}

	if (!*end)
		return 0;

	end++;
	if (*end != '.' && *end != 0)
		return 0;

	*tail = end;
	return 1;
}

/**
 * Check if a key component is valid, returning zero if invalid and non-zero if
 * valid.
 *
 * This family of functions is intended to be executed for each component of a
 * key to determine whether a config key literal is valid. These functions may
 * also be used to extract the individual components of a key.
 *
 * `comp_start` represents the start of a key component, which is expected to
 * begin with an alphanumeric character. `tail` is updated with a pointer to the
 * end of the component, which may be a period or null terminator.
 * */
static int is_valid_unquoted_key_component(const char *comp_start, const char **tail)
{
	const char *end;

	// read up to null or period
	for (end = comp_start; *end && *end != '.'; end++) {
		if (!isalnum(*end) && *end != '_')
			return 0;
	}

	if (*end != '.' && *end != 0)
		return 0;

	*tail = end;
	return 1;
}

/**
 * Determine if the next key component is valid.
 *
 * Reads up to a period or null character and checks whether this component
 * forms a valid key. Key components may be quoted with double quotes
 * if the key component contains non-alphanumeric characters.
 *
 * `tail` is updated with the end of the component (either a period or NULL character).
 * */
static int is_valid_key_component(const char *comp_start, const char **tail)
{
	if (!comp_start || !*comp_start)
		return 0;

	if (*comp_start == '"')
		return is_valid_quoted_key_component(comp_start, tail);

	return is_valid_unquoted_key_component(comp_start, tail);
}

int is_valid_config_key(const char *key)
{
	if (!key || !*key)
		return 0;

	const char *start = key;
	const char *end = key;
	while (*start) {
		int is_valid = is_valid_key_component(start, &end);
		if (!is_valid)
			return 0;

		// disallow empty components
		if (start == end)
			return 0;
		if (*end == '.' && !*(end + 1))
			return 0;

		start = end;
		if (*end)
			start = end + 1;
	}

	return 1;
}

int isolate_config_key_components(const char *key, struct str_array *components)
{
	if (!is_valid_config_key(key))
		return 1;

	struct strbuf component;
	strbuf_init(&component);

	const char *start = key;
	const char *end = NULL;
	while (*start) {
		// obtain pointer to the end of the key component
		is_valid_key_component(start, &end);

		const char *comp_start = start;
		const char *comp_end = end;
		if (*start == '"') {
			comp_start++;
			comp_end--;
		}

		strbuf_attach(&component, comp_start, comp_end - comp_start);
		unescape_buffer(&component);
		str_array_insert(components, component.buff, components->len);
		strbuf_clear(&component);

		start = end;
		if (*end)
			start = end + 1;
	}

	strbuf_release(&component);

	return 0;
}

int merge_config_key_components(struct str_array *components, struct strbuf *key)
{
	if (!components->len)
		return 1;

	struct strbuf key_tmp, comp_tmp;
	strbuf_init(&key_tmp);
	strbuf_init(&comp_tmp);

	const char *component = str_array_get(components, 0);
	strbuf_attach_str(&key_tmp, component);
	escape_buffer(&key_tmp);

	for (size_t i = 1; i < components->len; i++) {
		strbuf_clear(&comp_tmp);
		component = str_array_get(components, i);

		strbuf_attach_str(&comp_tmp, component);
		escape_buffer(&comp_tmp);

		strbuf_attach_fmt(&key_tmp, ".%s", comp_tmp.buff);
	}

	if (!is_valid_config_key(key_tmp.buff))
		return 1;

	if (key)
		strbuf_attach(key, key_tmp.buff, key_tmp.len);

	strbuf_release(&comp_tmp);
	strbuf_release(&key_tmp);
	return 0;
}

void escape_buffer(struct strbuf *buff)
{
	int should_quote = 0;
	for (size_t i = 0; i < buff->len; i++) {
		char c = buff->buff[i];
		if (!isalnum(c) && c != '_') {
			should_quote = 1;
			break;
		}
	}

	if (!should_quote)
		return;

	// replace all double quotes with \"
	// surround with double quotes
	struct strbuf comp_tmp;
	strbuf_init(&comp_tmp);

	strbuf_attach_chr(&comp_tmp, '"');

	for (size_t i = 0; i < buff->len; i++) {
		char c = buff->buff[i];

		if (c == '"')
			strbuf_attach_str(&comp_tmp, "\\\"");
		else if (c == '\\')
			strbuf_attach_str(&comp_tmp, "\\\\");
		else
			strbuf_attach_chr(&comp_tmp, c);
	}

	strbuf_attach_chr(&comp_tmp, '"');

	strbuf_clear(buff);
	strbuf_attach(buff, comp_tmp.buff, comp_tmp.len);

	strbuf_release(&comp_tmp);
}

void unescape_buffer(struct strbuf *buffer)
{
	// remove all character escapes
	char *escape = buffer->buff;
	while ((escape = memchr(escape, '\\', (buffer->buff + buffer->len) - escape))) {
		strbuf_remove(buffer, escape - buffer->buff, 1);
		escape++;
	}
}
