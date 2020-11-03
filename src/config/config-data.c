#include <stdlib.h>
#include <string.h>

#include "config/config-data.h"
#include "config/config-key.h"
#include "str-array.h"
#include "utils.h"

void config_data_init(struct config_data **config)
{
	struct config_data *new_config = (struct config_data *) malloc(sizeof(struct config_data));
	if (!new_config)
		FATAL(MEM_ALLOC_FAILED);

	*config = new_config;

	new_config->section = NULL;
	new_config->parent = NULL;
	str_array_init(&new_config->entries);
	new_config->entries.free_data = 1;

	new_config->subsections_len = 0;
	new_config->subsections_alloc = 8;
	new_config->subsections = (struct config_data **) calloc(new_config->subsections_alloc, sizeof(struct config_data *));
	if (!new_config->subsections)
		FATAL(MEM_ALLOC_FAILED);
}

void config_data_release(struct config_data **config)
{
	if (!config || !*config)
		return;
	if ((*config)->parent)
		BUG("config_data_release must only be invoked on root structure");

	struct config_data *current = *config;
	while (current) {
		struct config_data *tmp = NULL;

		// traverse as deep as possible and gradually release config data as we
		// move up the section hierarchy
		while (current->subsections_len) {
			tmp = current->subsections[current->subsections_len - 1];

			current->subsections[current->subsections_len - 1] = NULL;
			current->subsections_len--;

			current = tmp;
		}

		free(current->section);
		free(current->subsections);
		str_array_release(&current->entries);

		tmp = current;
		current = current->parent;
		free(tmp);
	}

	*config = NULL;
}

/**
 * Grow the subsections array such that it may fit at least `n` elements.
 * */
static void config_data_subsections_grow(struct config_data *config, size_t n)
{
	if (config->subsections_alloc >= n)
		return;

	config->subsections_alloc = config->subsections_alloc + 8;
	config->subsections = (struct config_data **) realloc(config->subsections, config->subsections_alloc);
	if (!config->subsections)
		FATAL(MEM_ALLOC_FAILED);
}

/**
 * Create or lookup a section hierarchy for the given config_data handle.
 * Returns a pointer to the new (or existing) config_data for that section
 * path.
 *
 * If `create_paths` is non-zero, missing sections are created. If zero,
 * this function will return NULL if no such section path exists.
 * */
static struct config_data *config_data_lookup_section(struct config_data *config,
		struct str_array *section_path, unsigned create_paths)
{
	struct config_data *current = config;
	for (size_t i = 0; i < section_path->len; i++) {
		char *comp = str_array_get(section_path, i);

		// check if subsection exists for this component
		int found = 0;
		for (size_t j = 0; j < current->subsections_len; j++) {
			char *subsection_header = current->subsections[j]->section;
			if (!strcmp(comp, subsection_header)) {
				found = 1;
				current = current->subsections[j];
				break;
			}
		}

		// if the subsection was not found, create it
		if (!found && !create_paths)
			return NULL;

		if (!found) {
			struct config_data *new_config;
			config_data_init(&new_config);

			// set new_config properties
			new_config->parent = current;
			new_config->section = strdup(comp);
			if (!new_config->section)
				FATAL(MEM_ALLOC_FAILED);

			config_data_subsections_grow(current, current->subsections_len + 1);
			current->subsections[current->subsections_len] = new_config;
			current->subsections_len++;

			current = new_config;
		}
	}

	return current;
}

/**
 * Try to locate a property in the given config_data. If found, returns the
 * entry and (optionally) updates `entry_index` with the index within the
 * string array. Otherwise returns NULL.
 * */
static struct str_array_entry *config_data_has_prop(struct config_data *config,
		const char *prop, size_t *entry_index)
{
	for (size_t i = 0; i < config->entries.len; i++) {
		struct str_array_entry *entry = str_array_get_entry(&config->entries, i);
		if (!strcmp(entry->string, prop)) {
			if (entry_index)
				*entry_index = i;

			return entry;
		}
	}

	return NULL;
}

int config_data_insert(struct config_data *config, const char *key, const char *value)
{
	struct str_array key_components;
	str_array_init(&key_components);

	int key_invalid = isolate_config_key_components(key, &key_components);
	if (key_invalid) {
		str_array_release(&key_components);
		return -1;
	}

	char *prop = str_array_remove(&key_components, key_components.len - 1);
	struct config_data *current = config_data_lookup_section(config, &key_components, 1);

	str_array_release(&key_components);

	// verify that there are no duplicate entries
	if (config_data_has_prop(current, prop, NULL)) {
		free(prop);
		return 1;
	}

	struct str_array_entry *entry = str_array_insert_nodup(&current->entries, prop, current->entries.len);
	entry->data = strdup(value);
	if (!entry->data)
		FATAL(MEM_ALLOC_FAILED);

	return 0;
}

int config_data_update(struct config_data *config, const char *key, const char *value)
{
	struct str_array key_components;
	str_array_init(&key_components);

	int key_invalid = isolate_config_key_components(key, &key_components);
	if (key_invalid) {
		str_array_release(&key_components);
		return -1;
	}

	char *prop = str_array_remove(&key_components, key_components.len - 1);
	struct config_data *current = config_data_lookup_section(config, &key_components, 0);
	str_array_release(&key_components);

	// if the section path does not exist, return
	if (!current) {
		free(prop);
		return 1;
	}

	// update the data, if property exists
	struct str_array_entry *prop_entry = config_data_has_prop(current, prop, NULL);
	if (!prop_entry) {
		free(prop);
		return 1;
	}

	free(prop_entry->data);
	prop_entry->data = strdup(value);
	if (!prop_entry->data)
		FATAL(MEM_ALLOC_FAILED);

	free(prop);
	return 0;
}

int config_data_delete(struct config_data *config, const char *key)
{
	struct str_array key_components;
	str_array_init(&key_components);

	int key_invalid = isolate_config_key_components(key, &key_components);
	if (key_invalid) {
		str_array_release(&key_components);
		return -1;
	}

	char *prop = str_array_remove(&key_components, key_components.len - 1);
	struct config_data *current = config_data_lookup_section(config, &key_components, 0);
	str_array_release(&key_components);

	// if the section path does not exist, return
	if (!current) {
		free(prop);
		return 1;
	}

	// delete the data, if property exists
	size_t index = 0;
	struct str_array_entry *prop_entry = config_data_has_prop(current, prop, &index);
	free(prop);

	if (!prop_entry)
		return 1;

	str_array_delete(&current->entries, index, 1);

	return 0;
}

const char *config_data_find(struct config_data *config, const char *key)
{
	struct str_array key_components;
	str_array_init(&key_components);

	int key_invalid = isolate_config_key_components(key, &key_components);
	if (key_invalid) {
		str_array_release(&key_components);
		return NULL;
	}

	char *prop = str_array_remove(&key_components, key_components.len - 1);
	struct config_data *current = config_data_lookup_section(config, &key_components, 0);
	str_array_release(&key_components);

	// if the section path does not exist, return
	if (!current) {
		free(prop);
		return NULL;
	}

	// return the prop value, if exists
	struct str_array_entry *prop_entry = config_data_has_prop(current, prop, NULL);
	free(prop);

	if (!prop_entry)
		return NULL;
	return prop_entry->data;
}

int config_data_get_section_key(struct config_data *config, struct strbuf *key)
{
	struct str_array sections;
	str_array_init(&sections);

	struct config_data *current = config;
	while (current) {
		if (current->section)
			str_array_insert(&sections, current->section, 0);
		current = current->parent;
	}

	int status = 1;
	if (sections.len) {
		int is_invalid = merge_config_key_components(&sections, key);
		if (is_invalid)
			goto fail;
	}

	status = 0;

fail:
	str_array_release(&sections);
	return status;
}
