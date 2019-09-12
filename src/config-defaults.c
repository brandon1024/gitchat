#include <string.h>
#include <ctype.h>

#include "config-defaults.h"

static const struct config_def config_defaults[] = {
		{ "channel.*.name", "" },
		{ "channel.*.createdby", "" },
		{ "channel.*.description", "" },
		{ NULL }
};

static int key_matches_pattern(const char *, const char *);

const char *get_default_config_value(const char *key)
{
	const struct config_def *config = config_defaults;
	while (config->key_pattern) {
		if (!strcmp(config->key_pattern, key))
			return config->default_value;
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
 	size_t key_index = 0, pattern_index = 0;
	do {
		if ((key[key_index] != 0) != (pattern[pattern_index] != 0))
			return 0;
		if (key[key_index] == 0 && pattern[pattern_index] == 0)
			break;

		if (pattern[pattern_index] == '*') {
			pattern_index++;
			while (isalnum(key[key_index]))
				key_index++;
		} else {
			if (key[key_index] != pattern[pattern_index])
				return 0;

			key_index++;
			pattern_index++;
		}
	} while (1);

	return 1;
}