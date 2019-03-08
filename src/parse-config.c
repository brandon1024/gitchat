#include <stdio.h>
#include <stdlib.h>

#include "parse-config.h"
#include "utils.h"

int parse_config(const char *conf_path, struct conf_data *conf)
{
	return 0;
}

char *find_value(struct conf_data *conf, const char *key)
{
	return NULL;
}

void release_config_resources(struct conf_data *conf)
{
	for(size_t i = 0; i < conf->len; i++) {
		free(conf->keys[i]);
		free(conf->values[i]);
	}

	free(conf->keys);
	free(conf->values);
}

int parse_config_callback(const char *conf_path, const char *key, void *data,
		void (*callback)(const char *key, const char *value, void *data))
{
	return 0;
}
