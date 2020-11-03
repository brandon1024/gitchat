#include <string.h>

#include "config/config-defaults.h"
#include "config/config-key.h"
#include "str-array.h"
#include "strbuf.h"

struct config_def {
	const char *key_pattern;
	const char *default_value;
};

static const struct config_def config_defaults[] = {
		{ "channel.*.name", "" },
		{ "channel.*.createdby", "" },
		{ "channel.*.description", "" },
		{ NULL, NULL }
};

/**
 * Determine whether a key matches a given pattern.
 *
 * Glob-like patterns are accepted, where '*' matches any number of alphanumeric
 * characters.
 *
 * Pattern: "example.*.key"
 * Matching: "example.section.key", "example.section123.key"
 * Not Matching: "example.key", "example..section1.section2.key"
 *
 * Returns 1 if the key matches the pattern, and zero otherwise.
 * */
static int key_matches_pattern(const char *pattern, const char *key)
{
	struct str_array key_components;
	str_array_init(&key_components);

	int is_invalid = isolate_config_key_components(key, &key_components);
	if (is_invalid) {
		str_array_release(&key_components);
		return 0;
	}

	struct str_array pattern_split;
	str_array_init(&pattern_split);

	struct strbuf pattern_buff;
	strbuf_init(&pattern_buff);

	strbuf_attach_str(&pattern_buff, pattern);
	strbuf_split(&pattern_buff, ".", &pattern_split);
	strbuf_release(&pattern_buff);

	if (pattern_split.len != key_components.len) {
		str_array_release(&key_components);
		str_array_release(&pattern_split);
		return 0;
	}

	for (size_t i = 0; i < pattern_split.len; i++) {
		const char *pattern_component = str_array_get(&pattern_split, i);
		const char *key_component = str_array_get(&key_components, i);

		if (!strcmp(pattern_component, "*"))
			continue;

		if (strcmp(key_component, pattern_component) != 0) {
			str_array_release(&key_components);
			str_array_release(&pattern_split);
			return 0;
		}
	}

	str_array_release(&key_components);
	str_array_release(&pattern_split);
	return 1;
}


const char *get_default_config_value(const char *key)
{
	const struct config_def *config = config_defaults;
	while (config->key_pattern) {
		if (key_matches_pattern(config->key_pattern, key))
			return config->default_value;

		config++;
	}

	return NULL;
}

int is_recognized_config_key(const char *key)
{
	return get_default_config_value(key) != NULL;
}
