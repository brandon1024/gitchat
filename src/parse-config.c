#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>

#include "parse-config.h"
#include "strbuf.h"
#include "utils.h"

#define BUFF_LEN 64
#define BUFF_SLOP 8

static char *read_line(FILE *fd);
static char *extract_section_name(char *line_str);
static int validate_section_name(char *section_name);
static int extract_key_value_pair(char *line_str, char **key, char **value);

int parse_config(struct conf_data *conf, const char *conf_path)
{
	struct stat sb;
	FILE *fd;
	char *current_section = NULL, *line;

	*conf = (struct conf_data){ .entries = NULL, .entries_len = 0, .entries_alloc = 0 };
	str_array_init(&conf->sections);

	if (stat(conf_path, &sb) == -1) {
		LOG_ERROR("Unable to stat file '%s'; %s", conf_path, strerror(errno));
		return -1;
	}

	fd = fopen(conf_path, "r");
	if (!fd) {
		LOG_ERROR("Cannot open file '%s'; %s", conf_path, strerror(errno));
		return -1;
	}

	while ((line = read_line(fd))) {
		char *section = extract_section_name(line);
		if (section) {
			if (validate_section_name(section)) {
				LOG_WARN("Invalid section name '%s'", section);

				free(section);
				free(line);
				release_config_resources(conf);
				return 1;
			}

			str_array_push(&conf->sections, section, NULL);
			current_section = str_array_get(&conf->sections, conf->sections.len - 1);
		} else {
			char *key = NULL;
			char *value = NULL;
			if (extract_key_value_pair(line, &key, &value)) {
				LOG_WARN("Could not parse line for key-value: '%s'", line);

				free(section);
				free(line);
				release_config_resources(conf);
				return 1;
			}

			// Ensure that no entries match keys
			for (size_t i = 0; i < conf->entries_len; i++) {
				struct conf_data_entry *entry = conf->entries[i];
				if (!current_section ^ !entry->section)
					continue;

				if (current_section && strcmp(current_section, entry->section) != 0)
					continue;

				if (!strcmp(key, conf->entries[i]->key)) {
					LOG_WARN("Duplicate keys found: '%s'", key);

					free(section);
					free(line);
					free(key);
					free(value);
					release_config_resources(conf);
					return 1;
				}
			}

			if ((conf->entries_len + 2) >= conf->entries_alloc) {
				conf->entries_alloc += BUFF_SLOP;
				conf->entries = (struct conf_data_entry **)realloc(conf->entries,
						sizeof(struct conf_data_entry *) * conf->entries_alloc);
				if(!conf->entries)
					FATAL("Unable to allocate memory.");
			}

			struct conf_data_entry *alloc_entry =
					(struct conf_data_entry *)malloc(sizeof(struct conf_data_entry));
			if (!alloc_entry)
				FATAL("Unable to allocate memory.");

			struct conf_data_entry entry = {current_section, key, value};
			conf->entries[conf->entries_len] = alloc_entry;
			*conf->entries[conf->entries_len++] = entry;
			conf->entries[conf->entries_len] = NULL;
		}

		free(section);
		free(line);
	}

	return 0;
}

char *find_value(struct conf_data *conf, char *key)
{
	size_t len = strlen(key);
	char *section_key_delim = key + len;

	//find section-key delimiter
	while (section_key_delim > key && *section_key_delim != '.')
		section_key_delim--;

	for (size_t i = 0; i < conf->entries_len; i++) {
		//if key does not have a section, simply compare key to entry key
		if (section_key_delim == key) {
			if (strcmp(key, conf->entries[i]->key) != 0)
				continue;

			return conf->entries[i]->value;
		}

		if (!conf->entries[i]->section)
			continue;

		//continue if section from entry does not match section
		if (strncmp(conf->entries[i]->section, key,
				(section_key_delim - key)) != 0)
			continue;

		//continue if key from entry does not match key
		if (strncmp(conf->entries[i]->key, section_key_delim + 1,
				(key + len - section_key_delim)) != 0)
			continue;

		return conf->entries[i]->value;
	}

	return NULL;
}

void release_config_resources(struct conf_data *conf)
{
	for (size_t i = 0; i < conf->entries_len; i++) {
		free(conf->entries[i]->key);
		free(conf->entries[i]->value);
		free(conf->entries[i]);
	}

	str_array_release(&conf->sections);
	free(conf->entries);
	*conf = (struct conf_data){ .entries = NULL, .entries_len = 0, .entries_alloc = 0 };
}

/**
 * Read an entire line from the file into memory, and return a pointer to it.
 * If EOF is reached, returns NULL. The string must be free()d by the caller.
 *
 * Leading and trailing whitespace is trimmed before the string is returned.
 * Lines consisting of whitespace only will be skipped.
 * */
static char *read_line(FILE *fd)
{
	char buffer[BUFF_LEN];
	size_t len = 0;
	struct strbuf buf;

	strbuf_init(&buf);

	char *eos;
	do {
		if (!fgets(buffer, BUFF_LEN, fd)) {
			strbuf_release(&buf);
			return NULL;
		}

		len = strlen(buffer);
		eos = buffer + len - 1;

		if (*eos == '\n' || feof(fd)) {
			strbuf_attach(&buf, buffer, (buffer - eos));

			char *ptr = buf.buff;
			while (*ptr && isspace(*ptr))
				ptr++;

			if (!*ptr) {
				strbuf_release(&buf);
				strbuf_init(&buf);
				eos = NULL;
			}
		} else {
			strbuf_attach(&buf, buffer, BUFF_LEN);
			eos = NULL;
		}
	} while(!eos);

	char *new_str = strbuf_detach(&buf);
	char *line_start = new_str;
	while (*line_start && isspace(*line_start))
		line_start++;

	char *line_end = memchr(new_str, 0, strlen(new_str) + 1);
	while ((line_end - 1) > line_start && isspace(*(line_end - 1)))
		line_end--;

	char *resized_str = (char *)calloc((line_end - line_start + 1), sizeof(char));
	if (!resized_str)
		FATAL("Unable to allocate memory.");

	len = line_end - line_start;
	strncpy(resized_str, line_start, len);

	free(new_str);

	return resized_str;
}

/**
 * Extract a section heading name, if applicable. If the heading is valid, a new
 * string is returned which represents the key for this heading. Otherwise, NULL
 * is returned.
 *
 * See parse-config.h for more details on config file format.
 * */
static char *extract_section_name(char *line_str)
{
	char *start, *end;

	size_t len = strlen(line_str);
	start = memchr(line_str, '[', len);
	if (!start)
		return NULL;

	end = line_str + len + 1;
	end = memchr(start, ']', (end - start));
	if (!end)
		return NULL;

	/* start and end must only be preceded/succeeded by whitespace */
	char *curr = start - 1;
	while (curr >= line_str) {
		if(isspace(*curr))
			return NULL;

		curr--;
	}

	curr = end + 1;
	while (curr < (line_str + len + 1)) {
		if(isspace(*curr))
			return NULL;

		curr++;
	}

	do
		start++;
	while (start < end && isspace(*start));

	if (start == end)
		return NULL;

	do
		end--;
	while (end > start && isspace(*end));

	len = end - start + 1;
	char *section_name = strndup(start, len);
	if (!section_name)
		FATAL("Unable to allocate memory.");

	return section_name;
}

/**
 * Simple validation function which determines if a section name is formatted
 * correctly, returning 0 if the section name is valid, and non-zero otherwise.
 *
 * See parse-config.h for more details on allowed section names.
 * */
static int validate_section_name(char *section_name)
{
	if (!section_name)
		return -1;

	size_t len = strlen(section_name);
	char *curr = section_name;

	while (curr < (section_name + len)) {
		if (!isalnum(*curr) && *curr != '_' && *curr != '.')
			return 1;

		curr++;
	}

	return 0;
}

/**
 * Attempt to extract a key value pair from a line taken from a config file.
 *
 * Allocates the memory necessary for the key and value string, and sets `key`
 * and `value` arguments with pointers to those strings.
 *
 * If the key or value cannot be extracted from the line string, returns
 * non-zero. Otherwise, returns zero.
 * */
static int extract_key_value_pair(char *line_str, char **key, char **value)
{
	char *key_start, *key_end, *val_start, *val_end;

	*key = NULL;
	*value = NULL;

	// Verify existence of `=`
	key_start = strchr(line_str, '=');
	if (!key_start)
		return -1;

	// Extract key from line
	key_start = line_str;
	while (*key_start && isspace(*key_start))
		key_start++;

	if (!*key_start || *key_start == '=') {
		LOG_TRACE("Unexpectedly reached end of string while parsing config line; '%s'", line_str);
		return -1;
	}

	key_end = strchr(line_str, '=');
	if (!key_end)
		BUG("unexpected NULL from strchr()");

	while ((key_end-1) > key_start && isspace(*(key_end-1)))
		key_end--;

	if (key_start == key_end) {
		LOG_TRACE("Config line missing key; '%s'", line_str);
		return -1;
	}

	for (char *s = key_start; s < key_end; s++) {
		if (!isalnum(*s) && *s != '.' && *s != '_') {
			LOG_TRACE("Disallowed character found in key; '%s'", line_str);
			return -1;
		}
	}

	// Extract value from line
	val_start = strchr(line_str, '=');
	if (!val_start)
		BUG("unexpected NULL from strchr()");

	do
		val_start++;
	while (*val_start && isspace(*val_start));

	val_end = line_str + strlen(line_str);
	while ((val_end - 1) > val_start && isspace(*val_end - 1))
		val_end--;

	*key = strndup(key_start, (key_end - key_start));
	*value = strndup(val_start, (val_end - val_start));

	return 0;
}
