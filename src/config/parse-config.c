#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "config/parse-config.h"
#include "config/config-data.h"
#include "config/config-key.h"
#include "config/config-defaults.h"
#include "config/node-visitor.h"
#include "strbuf.h"
#include "utils.h"

#define BUFF_LEN 1024

int parse_config(struct config_data *conf, const char *conf_path)
{
	int fd = open(conf_path, O_RDONLY);
	if (fd < 0)
		return -1;

	int status = parse_config_fd(conf, fd);

	close(fd);
	return status;
}

/**
 * From a line of data read from the config file, attempt to extract a section key.
 * If the line represents a valid section identifier, `key` is updated with the
 * section key.
 *
 * Leading or trailing whitespace characters are ignored, both outside and inside
 * the square brackets identifying the section.
 *
 * Empty sections are allowed (`[]`) which is used to indicate that the section
 * is reset.
 *
 * Returns zero if successful, and non-zero if the line does not represent a section.
 * */
static int extract_section_key(struct strbuf *line_buff, struct strbuf *key)
{
	struct strbuf temporary;
	strbuf_init(&temporary);

	strbuf_attach_str(&temporary, line_buff->buff);
	strbuf_trim(&temporary);

	// check if first and last characters are '[' and ']'
	if (temporary.len < 2)
		goto fail;
	if (temporary.buff[0] != '[')
		goto fail;
	if (temporary.buff[temporary.len - 1] != ']')
		goto fail;

	// extract section contents
	strbuf_remove(&temporary, 0, 1);
	strbuf_remove(&temporary, temporary.len - 1, 1);
	strbuf_trim(&temporary);

	// check if key is valid
	if (temporary.len && !is_valid_config_key(temporary.buff))
		goto fail;

	strbuf_clear(key);
	strbuf_attach_str(key, temporary.buff);
	strbuf_release(&temporary);
	return 0;

fail:
	strbuf_release(&temporary);
	return 1;
}

/**
 * Attempt to extract the `property` and `value` components from a buffer containing
 * the line `<prop> = <val>`.
 *
 * If successful, `prop` and `val` are updated accordingly. Otherwise, this function
 * returns non-zero and `prop` and `val` are not updated.
 * */
static int extract_property(struct strbuf *line, struct strbuf *prop, struct strbuf *val)
{
	int status = 1;
	struct strbuf temporary, property, value;
	strbuf_init(&temporary);
	strbuf_init(&property);
	strbuf_init(&value);

	strbuf_attach_str(&temporary, line->buff);
	strbuf_trim(&temporary);

	// verify that buffer does not contain unprintable characters
	for (size_t i = 0; i < temporary.len; i++) {
		if (!isprint(temporary.buff[i]))
			goto fail;
	}

	// property may be quoted; leave quoted, since it becomes part of the key
	unsigned quoted = temporary.buff[0] == '\'' || temporary.buff[0] == '"';

	char *prop_end;
	if (quoted) {
		char quote = temporary.buff[0];

		for (prop_end = temporary.buff + 1; *prop_end && *prop_end != quote; prop_end++) {
			// if backslash, skip the next character (or return if end of string)
			if (*prop_end == '\\')
				prop_end++;
		}

		if (*prop_end != quote)
			goto fail;

		prop_end++;
	} else {
		prop_end = memchr(temporary.buff, '=', temporary.len);
		if (!prop_end)
			goto fail;
	}

	strbuf_attach(&property, temporary.buff, prop_end - temporary.buff);
	strbuf_trim(&property);
	strbuf_remove(&temporary, 0, prop_end - temporary.buff);
	strbuf_trim(&temporary);

	if (temporary.buff[0] != '=')
		goto fail;

	strbuf_remove(&temporary, 0, 1);
	strbuf_trim(&temporary);

	// value may be quoted; if so, unquote
	quoted = temporary.buff[0] == '\'' || temporary.buff[0] == '"';
	if (quoted) {
		char quote = temporary.buff[0];
		if (temporary.buff[temporary.len - 1] != quote)
			goto fail;

		strbuf_remove(&temporary, 0, 1);
		strbuf_remove(&temporary, temporary.len - 1, 1);

		unescape_buffer(&temporary);
		strbuf_attach(&value, temporary.buff, temporary.len);
	} else {
		strbuf_attach(&value, temporary.buff, temporary.len);
		strbuf_trim(&value);
	}

	status = 0;
	strbuf_attach(prop, property.buff, property.len);
	strbuf_attach(val, value.buff, value.len);

fail:
	strbuf_release(&value);
	strbuf_release(&property);
	strbuf_release(&temporary);
	return status;
}

/**
 * Read a single line from the given file descriptor `fd` into the string buffer
 * `line`. This function is stateful though `input_buffer`; unprocessed data
 * is left in the buffer for the next invocation of this function.
 *
 * Lines are trimmed of leading and trailing whitespace. Lines consisting of
 * only whitespace are filtered.
 * */
static int read_line_fd(struct strbuf *input_buffer, struct strbuf *line, int fd)
{
	char tmp[BUFF_LEN];

	do {
		char *lf;
		ssize_t bytes_read;

		// while there's data left to read from fd and we don't have a full
		// line yet in the input_buffer, read from fd into input_buffer.
		do {
			bytes_read = xread(fd, tmp, BUFF_LEN);
			if (bytes_read < 0)
				FATAL("failed to read from file descriptor");
			if (bytes_read > 0)
				strbuf_attach(input_buffer, tmp, bytes_read);

			lf = memchr(input_buffer->buff, '\n', input_buffer->len);
		} while (!lf && bytes_read);

		// if there's nothing left to read from fd and no trailing line feed,
		// attach rest of buffer to `line`
		if (!lf) {
			strbuf_attach(line, input_buffer->buff, input_buffer->len);
			strbuf_trim(line);

			strbuf_clear(input_buffer);
			return line->len == 0;
		}

		strbuf_attach(line, input_buffer->buff, lf - input_buffer->buff);
		strbuf_trim(line);

		strbuf_remove(input_buffer, 0, lf - input_buffer->buff + 1);
	} while (!line->len);

	return line->len == 0;
}

int parse_config_fd(struct config_data *conf, int fd)
{
	struct strbuf input_buffer, line, current_section, property, property_val;
	strbuf_init(&input_buffer);
	strbuf_init(&line);
	strbuf_init(&current_section);
	strbuf_init(&property);
	strbuf_init(&property_val);

	int status = 0;
	while (!status && !read_line_fd(&input_buffer, &line, fd)) {
		// is this a section line?
		if (!extract_section_key(&line, &current_section)) {
			strbuf_clear(&line);
			continue;
		}

		// is this a property line ?
		if (!extract_property(&line, &property, &property_val)) {
			struct strbuf key;
			strbuf_init(&key);

			if (current_section.len) {
				strbuf_attach_fmt(&key, "%s.%s", current_section.buff, property.buff);
			} else {
				strbuf_attach_str(&key, property.buff);
			}

			status = config_data_insert(conf, key.buff, property_val.buff);
			strbuf_release(&key);

			strbuf_clear(&property);
			strbuf_clear(&property_val);

			if (status < 0) {
				LOG_ERROR("failed to parse config data; invalid key.");
				break;
			} else if (status > 0) {
				LOG_ERROR("failed to parse config data; property with same key already exists.");
				break;
			}

			strbuf_clear(&line);
			continue;
		}

		// this line cannot be parsed, so fail
		status = 1;
		LOG_ERROR("unexpected config file format; invalid line '%s'", line.buff);
	}

	strbuf_release(&property_val);
	strbuf_release(&property);
	strbuf_release(&current_section);
	strbuf_release(&line);
	strbuf_release(&input_buffer);

	return status;
}

int write_config(struct config_data *conf, const char *conf_path)
{
	int conf_fd = open(conf_path, O_TRUNC | O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (conf_fd < 0)
		return -1;

	int status = write_config_fd(conf, conf_fd);
	close(conf_fd);

	return status;
}

/**
 * Write the section heading and any properties for the given config_data node
 * to the given file descriptor `fd`.
 *
 * Returns zero if successful and non-zero otherwise.
 * */
static int write_for_node(struct config_data *node, int fd)
{
	struct strbuf section_key, line_out, esc_tmp;
	strbuf_init(&section_key);
	strbuf_init(&line_out);
	strbuf_init(&esc_tmp);

	int status = -1;

	if (config_data_get_section_key(node, &section_key)) {
		status = 1;
		goto fail;
	}

	// if we are in a section, write the section heading
	if (section_key.len) {
		strbuf_attach_fmt(&line_out, "[ %s ]\n", section_key.buff);

		if (xwrite(fd, line_out.buff, line_out.len) != line_out.len)
			goto fail;

		strbuf_clear(&line_out);
	}

	// iterate over all entries
	for (size_t entry_index = 0; entry_index < node->entries.len; entry_index++) {
		struct str_array_entry *entry = str_array_get_entry(&node->entries, entry_index);

		const char *property = entry->string;
		const char *value = entry->data;

		// indent if we are in a section
		if (section_key.len)
			strbuf_attach_chr(&line_out, '\t');

		// escape property
		strbuf_attach_str(&esc_tmp, property);
		escape_buffer(&esc_tmp);
		strbuf_attach_fmt(&line_out, "%s = ", esc_tmp.buff);
		strbuf_clear(&esc_tmp);

		// escape value
		strbuf_attach_str(&esc_tmp, value);
		escape_buffer(&esc_tmp);
		strbuf_attach_fmt(&line_out, "%s\n", esc_tmp.buff);
		strbuf_clear(&esc_tmp);

		if (xwrite(fd, line_out.buff, line_out.len) != line_out.len)
			goto fail;

		strbuf_clear(&line_out);
	}

	status = 0;

fail:
	strbuf_release(&section_key);
	strbuf_release(&line_out);
	strbuf_release(&esc_tmp);
	return status;
}

int write_config_fd(struct config_data *conf, int fd)
{
	struct strbuf section, key;
	strbuf_init(&section);
	strbuf_init(&key);

	struct str_array sections;
	str_array_init(&sections);

	struct cd_node_visitor *visitor;
	node_visitor_init(&visitor, conf);

	int status = 0;
	struct config_data *node;
	while (!node_visitor_next(visitor, &node)) {
		if (!node->entries.len)
			continue;

		status = write_for_node(node, fd);
		if (status)
			break;
	}

	node_visitor_release(visitor);

	str_array_release(&sections);
	strbuf_release(&section);
	strbuf_release(&key);

	return status;
}

static int ensure_recognized_keys_only(struct config_data *conf)
{
	struct cd_node_visitor *visitor;
	node_visitor_init(&visitor, conf);

	struct strbuf key;
	strbuf_init(&key);

	int status = 0;
	struct config_data *node;
	while (!node_visitor_next(visitor, &node)) {
		if (!node->entries.len)
			continue;

		if (config_data_get_section_key(node, &key)) {
			status = -1;
			break;
		}

		struct strbuf prop_key;
		strbuf_init(&prop_key);

		for (size_t i = 0; i < node->entries.len; i++) {
			const char *prop_name = str_array_get(&node->entries, i);

			if (key.len)
				strbuf_attach_fmt(&prop_key, "%s.%s", key.buff, prop_name);
			else
				strbuf_attach_str(&prop_key, prop_name);

			if (!is_recognized_config_key(prop_key.buff)) {
				status = 2;
				break;
			}

			strbuf_clear(&prop_key);
		}

		strbuf_release(&prop_key);
		if (status == 2)
			break;

		strbuf_clear(&key);
	}

	strbuf_release(&key);
	node_visitor_release(visitor);
	return status;
}

int is_config_invalid(const char *conf_path, int recognized_keys_only)
{
	struct config_data *conf;

	config_data_init(&conf);
	int status = parse_config(conf, conf_path);
	if (status != 0) {
		config_data_release(&conf);
		return -1;
	}

	if (recognized_keys_only)
		status = ensure_recognized_keys_only(conf);

	config_data_release(&conf);
	return status;
}
