#ifndef GIT_CHAT_INCLUDE_CONFIG_CONFIG_DEFAULTS_H
#define GIT_CHAT_INCLUDE_CONFIG_CONFIG_DEFAULTS_H

/**
 * Fetch from the default configuration definitions the statically-allocated
 * default value for a given key.
 *
 * If found, returns the default value, otherwise returns NULL.
 * */
const char *get_default_config_value(const char *key);

/**
 * Determine whether a given config key is recognized by the application.
 *
 * If recognized, returns 1. Otherwise returns 0.
 * */
int is_recognized_config_key(const char *key);

#endif //GIT_CHAT_INCLUDE_CONFIG_CONFIG_DEFAULTS_H
