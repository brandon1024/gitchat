#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include "parse-config.h"
#include "config-defaults.h"
#include "strbuf.h"
#include "utils.h"

#define BUFF_LEN 1024

struct config_entry {
	struct config_entry *next;
	struct config_entry *prev;
	char *key;
	char *value;
};

static char *read_line(FILE *);
static char *extract_section_name(char *);
static struct config_entry *create_config_data_entry(const char *, const char *);
static void config_file_data_insert_existing_entry(struct config_file_data *,
		struct config_entry *);
static void release_config_data_entry(struct config_entry *);
static void sort_config_file_data_entries(struct config_file_data *);
static int has_duplicate_entries(struct config_file_data *, int);
static void config_file_data_remove_entry(struct config_file_data *,
		struct config_entry *);

void config_file_data_init(struct config_file_data *conf)
{
	*conf = (struct config_file_data){
			.head = NULL,
			.tail = NULL
	};
}

int parse_config(struct config_file_data *conf, const char *conf_path)
{
	struct stat sb;
	FILE *fp;

	config_file_data_init(conf);

	if (stat(conf_path, &sb) == -1) {
		LOG_ERROR("Unable to stat file '%s'; %s", conf_path, strerror(errno));
		return -1;
	}

	fp = fopen(conf_path, "r");
	if (!fp) {
		LOG_ERROR("Cannot open file '%s'; %s", conf_path, strerror(errno));
		return -1;
	}

	char *line;
	char *current_section = NULL;
	while ((line = read_line(fp))) {
		char *section = extract_section_name(line);
		if (section) {
			free(current_section);
			current_section = section;
		} else {
			struct config_entry *entry = create_config_data_entry(current_section, line);
			if (!entry) {
				LOG_WARN("Invalid config line '%s' or section '%s'", line, current_section);

				free(current_section);
				free(line);
				fclose(fp);
				config_file_data_release(conf);
				return 1;
			}

			config_file_data_insert_existing_entry(conf, entry);
		}

		free(line);
	}

	free(current_section);
	fclose(fp);

	return 0;
}

int write_config(struct config_file_data *conf, const char *conf_path)
{
	// sort the list of entries
	sort_config_file_data_entries(conf);

	/*
	 * Check for duplicate keys. No need to validate keys here, since all other
	 * functions that create entries must check for valid keys.
	 * */
	if (has_duplicate_entries(conf, 1))
		return 1;

	int conf_fd = open(conf_path, O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (conf_fd < 0)
		return -1;

	struct strbuf temporary;
	strbuf_init(&temporary);

	struct strbuf current_section;
	strbuf_init(&current_section);

	struct config_entry *entry = conf->head;
	while (entry) {
		char *entry_key = entry->key;
		char *section_token = strrchr(entry_key, '.');

		if (section_token && strncmp(entry_key, current_section.buff, (section_token - entry_key)) != 0) {
			strbuf_clear(&current_section);
			strbuf_attach(&current_section, entry_key, (section_token - entry_key));

			strbuf_clear(&temporary);
			strbuf_attach_fmt(&temporary, "[%s]\n", current_section.buff);

			if (recoverable_write(conf_fd, temporary.buff, temporary.len) != temporary.len)
				FATAL(FILE_WRITE_FAILED, conf_path);
		}

		strbuf_clear(&temporary);
		if (section_token)
			strbuf_attach_fmt(&temporary, "\t%s = ", (section_token + 1));
		else
			strbuf_attach_fmt(&temporary, "%s = ", entry->key);

		int has_lead_trail_space = 0;
		if (isspace(*entry->value))
			has_lead_trail_space = 1;
		if (*entry->value && isspace(*(entry->value + strlen(entry->value) - 1)))
			has_lead_trail_space = 1;

		if (has_lead_trail_space)
			strbuf_attach_fmt(&temporary, "\"%s\"\n", entry->value);
		else
			strbuf_attach_fmt(&temporary, "%s\n", entry->value);

		if (recoverable_write(conf_fd, temporary.buff, temporary.len) != temporary.len)
			FATAL(FILE_WRITE_FAILED, conf_path);

		entry = entry->next;
	}

	strbuf_release(&current_section);
	strbuf_release(&temporary);
	close(conf_fd);

	return 0;
}

int is_config_invalid(const char *conf_path, int recognized_keys_only)
{
	struct config_file_data conf;
	int status = parse_config(&conf, conf_path);
	if (status != 0)
		return status;

	if (has_duplicate_entries(&conf, 0)) {
		config_file_data_release(&conf);
		return 1;
	}

	if (recognized_keys_only) {
		struct config_entry *entry = conf.head;
		while (entry) {
			if (!is_valid_key(entry->key) || !is_recognized_config_key(entry->key)) {
				config_file_data_release(&conf);
				return 2;
			}

			entry = entry->next;
		}
	}

	config_file_data_release(&conf);
	return 0;
}

void config_file_data_release(struct config_file_data *conf)
{
	struct config_entry *entry = conf->head;
	while (entry) {
		struct config_entry *next = entry->next;

		release_config_data_entry(entry);
		entry = next;
	}

	conf->head = NULL;
	conf->tail = NULL;
}

struct config_entry *config_file_data_insert_entry(struct config_file_data *conf,
		const char *key, const char *value)
{
	if (!is_valid_key(key))
		return NULL;

	struct config_entry *entry = (struct config_entry *)malloc(sizeof(struct config_entry));
	if (!entry)
		FATAL(MEM_ALLOC_FAILED);

	char *new_key = strdup(key);
	if (!new_key)
		FATAL(MEM_ALLOC_FAILED);

	char *new_value = strdup(value);
	if (!new_value)
		FATAL(MEM_ALLOC_FAILED);

	*entry = (struct config_entry){
			.key = new_key,
			.value = new_value,
			.next = NULL,
			.prev = NULL
	};

	config_file_data_insert_existing_entry(conf, entry);

	return entry;
}

void config_file_data_delete_entry(struct config_file_data *conf, struct config_entry *entry)
{
	config_file_data_remove_entry(conf, entry);
	release_config_data_entry(entry);
}

struct config_entry *config_file_data_find_entry(struct config_file_data *conf, const char *key)
{
	struct config_entry *entry = conf->head;
	while (entry) {
		if (!strcmp(entry->key, key))
			return entry;

		entry = entry->next;
	}

	return NULL;
}

char *config_file_data_get_entry_value(struct config_entry *entry)
{
	return entry->value;
}

void config_file_data_set_entry_value(struct config_entry *entry, const char *value)
{
	if (!value)
		BUG("config_file_data_set_entry_value() value must be nonnull");

	free(entry->value);
	entry->value = strdup(value);
	if (!entry->value)
		FATAL(MEM_ALLOC_FAILED);
}

char *config_file_data_get_entry_key(struct config_entry *entry)
{
	return entry->key;
}

/**
 * Check that a given key string is valid, returning zero of valid and non-zero
 * if invalid.
 *
 * A valid key may:
 * - be alphanumeric characters, including '.' and '_'
 * - each '.' must be separated by another character
 * */
int is_valid_key(const char *key)
{
	if (!strlen(key))
		return 0;

	if (*key == '.')
		return 0;

	// check to see if each '.' is separated by an alphanumeric char
	const char *c = key;
	while ((c = strchr(c, '.'))) {
		c++;
		if (!isalnum(*c) && *c != '_')
			return 0;
	}

	// check to see if key has any non-alphanumeric character, (excluding '.' and '_')
	c = key;
	while (*c) {
		if (!isalnum(*c) && *c != '.' && *c != '_')
			return 0;
		c++;
	}

	return 1;
}


/**
 * Read an entire line from the file into memory, and return a pointer to it.
 * If EOF is reached, returns NULL. The string must be free()d by the caller.
 *
 * Leading and trailing whitespace is trimmed before the string is returned.
 * Lines consisting of whitespace only will be skipped.
 * */
static char *read_line(FILE *fp)
{
	char buffer[BUFF_LEN];
	struct strbuf buf;

	strbuf_init(&buf);

	char *eos;
	do {
		if (!fgets(buffer, BUFF_LEN, fp)) {
			strbuf_release(&buf);
			return NULL;
		}

		eos = buffer + strlen(buffer) - 1;
		if (*eos == '\n' || feof(fp)) {
			strbuf_attach(&buf, buffer, (eos - buffer));
			strbuf_trim(&buf);

			if (!buf.len)
				eos = NULL;
		} else {
			strbuf_attach(&buf, buffer, BUFF_LEN);
			eos = NULL;
		}
	} while (!eos);

	strbuf_trim(&buf);

	return strbuf_detach(&buf);
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

	// start and end must only be preceded/succeeded by whitespace
	char *curr = start - 1;
	while (curr >= line_str) {
		if (isspace(*curr))
			return NULL;

		curr--;
	}

	curr = end + 1;
	while (curr < (line_str + len + 1)) {
		if (isspace(*curr))
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
		FATAL(MEM_ALLOC_FAILED);

	return section_name;
}

/**
 * Create a config data entry from a line extracted from the configuration file and the associated
 * section that the line belongs to.
 *
 * If the line failed to parse for any reason, the function will return a null pointer.
 * Otherwise, returns a newly allocated config_entry.
 * */
static struct config_entry *create_config_data_entry(const char *section, const char *line_str)
{
	// Verify existence of `=`
	char *eq_char = strchr(line_str, '=');
	if (!eq_char)
		return NULL;

	struct strbuf key;
	strbuf_init(&key);

	if (section)
		strbuf_attach_fmt(&key, "%s.", section);

	strbuf_attach(&key, line_str, (eq_char - line_str));
	strbuf_trim(&key);

	if (!is_valid_key(key.buff)) {
		strbuf_release(&key);
		return NULL;
	}

	struct strbuf value_buff;
	strbuf_init(&value_buff);
	strbuf_attach_str(&value_buff, eq_char + 1);
	strbuf_trim(&value_buff);

	// if the value is quoted, unquote
	size_t value_len = value_buff.len;
	char *value = strbuf_detach(&value_buff);
	if ((*value == '\'' && *(value + value_len - 1) == '\'') ||
		(*value == '"' && *(value + value_len - 1) == '"')) {
		memmove(value, value + 1, --value_len);
		value[value_len--] = 0;
		value[value_len] = 0;
	}

	// create the entry
	struct config_entry *entry = (struct config_entry *)malloc(sizeof(struct config_entry));
	if (!entry)
		FATAL(MEM_ALLOC_FAILED);

	*entry = (struct config_entry){
			.key = strbuf_detach(&key),
			.value = value,
			.next = NULL,
			.prev = NULL
	};

	return entry;
}

/**
 * Insert a config_entry that has already been allocated into the config_file_data
 * linked list of config entries.
 * */
static void config_file_data_insert_existing_entry(struct config_file_data *conf,
		struct config_entry *entry)
{
	if (!conf->tail) {
		entry->next = NULL;
		entry->prev = NULL;

		conf->head = entry;
		conf->tail = entry;
	} else {
		entry->prev = conf->tail;
		entry->next = NULL;

		conf->tail->next = entry;
		conf->tail = entry;
	}
}

/**
 * Release any resources under a config_data_entry structure.
 * */
static void release_config_data_entry(struct config_entry *entry)
{
	free(entry->key);
	free(entry->value);
	free(entry);
}

/**
 * Sort all entries in the config_file_data in strcmp() order.
 * */
static void sort_config_file_data_entries(struct config_file_data *conf)
{
	struct config_file_data tmp_conf;
	config_file_data_init(&tmp_conf);

	while (conf->head) {
		struct config_entry *current = conf->head;
		struct config_entry *smallest = current;

		// find next smallest in strcmp() order
		while (current) {
			if (!strchr(current->key, '.') && strchr(smallest->key, '.') != NULL)
				smallest = current;
			if (strchr(current->key, '.') != NULL && !strchr(smallest->key, '.')) {
				current = current->next;
				continue;
			}

			if (!strcmp(current->key, smallest->key)) {
				if (strcmp(current->value, smallest->value) < 0)
					smallest = current;
			} else if (strcmp(current->key, smallest->key) < 0) {
				smallest = current;
			}

			current = current->next;
		}

		// remove from existing config data
		config_file_data_remove_entry(conf, smallest);

		// append to new (temporary) config data
		config_file_data_insert_existing_entry(&tmp_conf, smallest);
	}

	conf->head = tmp_conf.head;
	conf->tail = tmp_conf.tail;
}

/**
 * Check if the config_file_data structure has any entries with duplicate keys.
 * */
static int has_duplicate_entries(struct config_file_data *conf, int is_sorted)
{
	struct config_entry *current = conf->head;
	while (current) {
		if (is_sorted) {
			/*
			 * Since we assume this is a sorted list, if current and next are not
			 * equal, then we can assume that there are no duplicates for
			 * in the list for current, so skip to the next.
			 */
			if (current->next && !strcmp(current->key, current->next->key))
				return 1;
		} else {
			struct config_entry *next = conf->head;
			while (next) {
				if (next != current && !strcmp(current->key, next->key))
					return 1;

				next = next->next;
			}
		}

		current = current->next;
	}

	return 0;
}

/**
 * Remove an entry from a conf_file_data without releasing the entry resources.
 * */
static void config_file_data_remove_entry(struct config_file_data *conf, struct config_entry *entry)
{
	if (conf->head == NULL || conf->tail == NULL)
		return;

	if (conf->head == entry || conf->tail == entry) {
		if (conf->head == entry)
			conf->head = entry->next;

		if (conf->tail == entry)
			conf->tail = entry->prev;
	}

	if (entry->next)
		entry->next->prev = entry->prev;

	if (entry->prev)
		entry->prev->next = entry->next;

	entry->next = NULL;
	entry->prev = NULL;
}
