#ifndef GIT_CHAT_INCLUDE_CONFIG_CONFIG_DATA_H
#define GIT_CHAT_INCLUDE_CONFIG_CONFIG_DATA_H

#include <stddef.h>

#include "str-array.h"
#include "strbuf.h"

/**
 * Overview of Config Data
 *
 * At it's most fundamental level, git-chat config files are represented in
 * terms of keys and values. Keys are used to define the path to a property in
 * nested sections.
 *
 * The config_data structure represents this data hierarchically. The value of
 * a key can be found by traversing the sections / subsections of a config_data
 * structure.
 * */

struct config_data {
	char *section;
	struct config_data *parent;
	struct config_data **subsections;
	struct str_array entries;
	size_t subsections_len;
	size_t subsections_alloc;
};

/**
 * Allocate and initialize a config_data structure with default values.
 * `config` is updated with a pointer to the new structure.
 * */
void config_data_init(struct config_data **config);

/**
 * Release all resources under a config_data structure.
 * */
void config_data_release(struct config_data **config);

/**
 * Insert a config value into the config_data at a given key path.
 *
 * If the key is invalid, -1 is returned. If the key points to a config that
 * already exists, returns 1. Otherwise, returns zero to indicate that the
 * value was successfully inserted.
 * */
int config_data_insert(struct config_data *config, const char *key, const char *value);

/**
 * Insert a config value into the config_data structure at an exploded key
 * (constructed from variadic arguments).
 *
 * If the key is invalid, -1 is returned. If the key points to a config that
 * already exists, returns 1. Otherwise, returns zero to indicate that the
 * value was successfully inserted.
 * */
__attribute__ ((sentinel))
int config_data_insert_exp_key(struct config_data *config, const char *value, ...);

/**
 * Update a config value in the config_data at a given key path.
 *
 * If the key is invalid, -1 is returned. If the key points to a config that
 * does not exist, returns 1. Otherwise, returns zero to indicate that the
 * value was successfully updated.
 * */
int config_data_update(struct config_data *config, const char *key, const char *value);

/**
 * Update a config value in the config_data at an exploded key
 * (constructed from variadic arguments).
 *
 * If the key is invalid, -1 is returned. If the key points to a config that
 * does not exist, returns 1. Otherwise, returns zero to indicate that the
 * value was successfully updated.
 * */
__attribute__ ((sentinel))
int config_data_update_exp_key(struct config_data *config, const char *value, ...);

/**
 * Remove a config value from the config_data at a given key path.
 *
 * If the key is invalid, -1 is returned. If the key points to a config that
 * does not exist, returns 1. Otherwise, returns zero to indicate that the
 * value was successfully removed.
 * */
int config_data_delete(struct config_data *config, const char *key);

/**
 * Remove a config value from the config_data at an exploded key
 * (constructed from variadic arguments).
 *
 * If the key is invalid, -1 is returned. If the key points to a config that
 * does not exist, returns 1. Otherwise, returns zero to indicate that the
 * value was successfully removed.
 * */
__attribute__ ((sentinel))
int config_data_delete_exp_key(struct config_data *config, ...);

/**
 * Retrieve the value for a config property with the given key.
 *
 * If the key is invalid, or the key points to a config that
 * does not exist, returns NULL. Otherwise, returns a pointer to the config value.
 * */
const char *config_data_find(struct config_data *config, const char *key);

/**
 * Retrieve the value for a config property with the given (exploded) key.
 *
 * If the key is invalid, or the key points to a config that
 * does not exist, returns NULL. Otherwise, returns a pointer to the config value.
 * */
__attribute__ ((sentinel))
const char *config_data_find_exp_key(struct config_data *config, ...);

/**
 * Retrieve the section key for a particular config_data node.
 *
 * If `config` is the root node, `key` is not updated. Otherwise, key is updated
 * with the key for the given node, with the format `<section 1>.<...>.<section n>`.
 *
 * Returns zero if successful, or non-zero if the key could not be reconstructed.
 * */
int config_data_get_section_key(struct config_data *config, struct strbuf *key);

#endif //GIT_CHAT_INCLUDE_CONFIG_CONFIG_DATA_H
