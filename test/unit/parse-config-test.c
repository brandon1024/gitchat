#include <unistd.h>
#include <string.h>

#include "test-lib.h"
#include "config/parse-config.h"
#include "utils.h"

static int write_conf_to_fd(const char data[], size_t len)
{
	int fd[2];
	if (pipe(fd) < 0)
		return -1;

	ssize_t bout = xwrite(fd[1], data, len);
	close(fd[1]);

	if (bout != len) {
		close (fd[0]);
		return -1;
	}

	return fd[0];
}

static int read_fd_to_strbuf(int fd, struct strbuf *buff)
{
	char tmp[1024];
	ssize_t bytes_read = 0;
	while ((bytes_read = xread(fd, tmp, 1024)) > 0)
		strbuf_attach(buff, tmp, bytes_read);

	if (bytes_read < 0)
		return -1;

	return 0;
}

TEST_DEFINE(parse_config_ignore_empty_lines_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[section]\n"
			"    property = value1\n"
			"\n"
			"[section.subsection]\n"
			"\n"
			"    property = value2\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_string_eq("value1", config_data_find(conf, "section.property"));
		assert_string_eq("value2", config_data_find(conf, "section.subsection.property"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_empty_file_test)
{
	struct config_data *conf;
	const char *config_file_empty = "";
	const char *config_file_whitespace = "\n     \n \n\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_empty, strlen(config_file_empty));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		// test empty file
		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		close(fd);
		fd = write_conf_to_fd(config_file_whitespace, strlen(config_file_whitespace));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		// test file with only whitespace
		status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_section_trim_outer_whitespace_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"     [section] \n"
			"    property = value1\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_eq_msg(1, conf->subsections_len, "unexpected number of subsections");
		assert_string_eq("section", conf->subsections[0]->section);
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_section_trim_inner_whitespace_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[  section  ]\n"
			"    property = value1\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_eq_msg(1, conf->subsections_len, "unexpected number of subsections");
		assert_string_eq("section", conf->subsections[0]->section);
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_section_no_properties_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[ section ]\n"
			"[ section.subsection ]\n"
			"    property = value1\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_zero_msg(conf->entries.len, "unexpected number of entries");
		assert_eq_msg(1, conf->subsections_len, "unexpected number of subsections");

		struct config_data *tmp = conf->subsections[0];
		assert_string_eq("section", tmp->section);
		assert_zero_msg(tmp->entries.len, "unexpected number of entries");
		assert_eq_msg(1, tmp->subsections_len, "unexpected number of subsections");

		tmp = tmp->subsections[0];
		assert_string_eq("subsection", tmp->section);
		assert_eq_msg(1, tmp->entries.len, "unexpected number of entries");
		assert_zero_msg(tmp->subsections_len, "unexpected number of subsections");
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_section_no_section_name_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"property = value1\n"
			"[ section.subsection ]\n"
			"    property = value1\n"
			"[  ]\n"
			"    property1 = value1\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_eq_msg(2, conf->entries.len, "unexpected number of entries");
		str_array_sort(&conf->entries);

		assert_string_eq("property", str_array_get(&conf->entries, 0));
		assert_string_eq("property1", str_array_get(&conf->entries, 1));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_section_simple_section_name_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[ section.subsection ]\n"
			"    property = value1\n"
			"[ section.new.sub_section ]\n"
			"    property = value2\n"
			"[ section.subsection ]\n"
			"    property1 = value3\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_string_eq("value1",  config_data_find(conf, "section.subsection.property"));
		assert_string_eq("value2",  config_data_find(conf, "section.new.sub_section.property"));
		assert_string_eq("value3",  config_data_find(conf, "section.subsection.property1"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_section_quoted_section_name_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[ \"sec.tion\".subsection ]\n"
			"    property = value1\n"
			"[ section.\"ne-w\".subsection ]\n"
			"    property = value2\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_eq_msg(2, conf->subsections_len, "unexpected number of subsections");

		struct config_data *tmp = conf->subsections[0];
		assert_string_eq("sec.tion", tmp->section);
		assert_string_eq("value1", config_data_find(conf, "\"sec.tion\".subsection.property"));

		tmp = conf->subsections[1];
		assert_string_eq("section", tmp->section);
		assert_eq_msg(1, tmp->subsections_len, "unexpected number of subsections");

		tmp = tmp->subsections[0];
		assert_string_eq("ne-w", tmp->section);
		assert_eq_msg(1, tmp->subsections_len, "unexpected number of subsections");
		assert_string_eq("value2", config_data_find(conf, "section.\"ne-w\".subsection.property"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_section_escaped_chars_section_name_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[ \"sec\\\\tion\".subsection ]\n"
			"    property = value1\n"
			"[ section.\"ne\\\"w\".subsection ]\n"
			"    property = value2\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_eq_msg(2, conf->subsections_len, "unexpected number of subsections");

		struct config_data *tmp = conf->subsections[0];
		assert_string_eq("sec\\tion", tmp->section);
		assert_string_eq("value1", config_data_find(conf, "\"sec\\\\tion\".subsection.property"));

		tmp = conf->subsections[1];
		assert_string_eq("section", tmp->section);
		assert_eq_msg(1, tmp->subsections_len, "unexpected number of subsections");

		tmp = tmp->subsections[0];
		assert_string_eq("ne\"w", tmp->section);
		assert_eq_msg(1, tmp->subsections_len, "unexpected number of subsections");
		assert_string_eq("value2", config_data_find(conf, "section.\"ne\\\"w\".subsection.property"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_section_invalid_name_fail_test)
{
	struct config_data *conf;
	const char *invalid1 =
			"[ \"sec\\\\tion\".subsection ]\n"
			"    property = value1\n"
			"[ section-name ]\n"
			"    property = value2\n";
	const char *invalid2 =
			"[section] this is invalid\n"
			"    property = value\n";
	const char *invalid3 =
			"[section stuff]\n"
			"    property = value\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(invalid1, strlen(invalid1));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_nonzero_msg(status, "parse_config_fd returned zero for invalid config");

		close(fd);
		fd = write_conf_to_fd(invalid2, strlen(invalid2));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		status = parse_config_fd(conf, fd);
		assert_nonzero_msg(status, "parse_config_fd returned zero for invalid config");

		close(fd);
		fd = write_conf_to_fd(invalid3, strlen(invalid3));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		status = parse_config_fd(conf, fd);
		assert_nonzero_msg(status, "parse_config_fd returned zero for invalid config");
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_property_invalid_test)
{
	struct config_data *conf;
	const char *invalid1 =
			"[ section ]\n"
			"    = value1\n";
	const char *invalid2 =
			"[ section ]\n"
			"    property 1 = value1\n";
	const char *invalid3 =
			"[ section ]\n"
			"    property^subprop = value1\n";
	const char *invalid4 =
			"[ section ]\n"
			"    property\n";
	const char *invalid5 =
			"[ section ]\n"
			"    property = \"my value contains line feed\n\"\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(invalid1, strlen(invalid1));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_nonzero_msg(status, "parse_config_fd should fail when property name missing");

		close(fd);
		fd = write_conf_to_fd(invalid2, strlen(invalid2));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		status = parse_config_fd(conf, fd);
		assert_nonzero_msg(status, "parse_config_fd should fail when property name contains spaces");

		close(fd);
		fd = write_conf_to_fd(invalid3, strlen(invalid3));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		status = parse_config_fd(conf, fd);
		assert_nonzero_msg(status, "parse_config_fd should fail if property name contains period");

		close(fd);
		fd = write_conf_to_fd(invalid4, strlen(invalid4));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		status = parse_config_fd(conf, fd);
		assert_nonzero_msg(status, "parse_config_fd should fail if property value missing");

		close(fd);

		fd = write_conf_to_fd(invalid5, strlen(invalid5));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		status = parse_config_fd(conf, fd);
		assert_nonzero_msg(status, "parse_config_fd should fail if property value missing");

		close(fd);
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_property_trim_whitespace_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[ section ]\n"
			"    property1=  value1\n"
			"    property2 =value2  \n"
			"    property3   =    value3   \n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_string_eq("value1",  config_data_find(conf, "section.property1"));
		assert_string_eq("value2",  config_data_find(conf, "section.property2"));
		assert_string_eq("value3",  config_data_find(conf, "section.property3"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_property_quoted_prop_names_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[ section ]\n"
			"    \"property-1\" = value1\n"
			"    \"property\\\"2\" = value2\n"
			"    \"property 3\" = \\\"value3\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");
		assert_eq_msg(1, conf->subsections_len, "unexpected number of subsections");

		struct config_data *tmp = conf->subsections[0];
		assert_string_eq("section", tmp->section);
		assert_eq_msg(3, tmp->entries.len, "unexpected number of properties for section");

		str_array_sort(&tmp->entries);
		assert_string_eq("property 3", str_array_get(&tmp->entries, 0));
		assert_string_eq("property\"2", str_array_get(&tmp->entries, 1));
		assert_string_eq("property-1", str_array_get(&tmp->entries, 2));

		assert_string_eq("value1",  config_data_find(conf, "section.\"property-1\""));
		assert_string_eq("value2",  config_data_find(conf, "section.\"property\\\"2\""));
		assert_string_eq("\\\"value3",  config_data_find(conf, "section.\"property 3\""));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_property_quoted_prop_values_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[ section ]\n"
			"    \"prop 1\" = value1\n"
			"    \" property-2 \" = value2\n"
			"    property3 = value3\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_string_eq("value1",  config_data_find(conf, "section.\"prop 1\""));
		assert_string_eq("value2",  config_data_find(conf, "section.\" property-2 \""));
		assert_string_eq("value3",  config_data_find(conf, "section.property3"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_property_char_escape_prop_names_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[ section ]\n"
			"    \"prop\\\\1\" = value1\n"
			"    \" property \\\" 2 \" = value2\n"
			"    \"property\\3\" = value3\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_string_eq("value1",  config_data_find(conf, "section.\"prop\\\\1\""));
		assert_string_eq("value2",  config_data_find(conf, "section.\" property \\\" 2 \""));
		assert_string_eq("value3",  config_data_find(conf, "section.property3"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_property_char_escape_prop_values_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"[ section ]\n"
			"    property1 = this % is * a value with $symbols\n"
			"    property2 = \"this value is quoted, quotes are removed\"\n"
			"    property3 =\"this value is quoted, \\\" quotes are removed\"  \n"
			"    property4 = \"this value contains =\"\n"
			"    property5 = \n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_string_eq("this % is * a value with $symbols",  config_data_find(conf, "section.property1"));
		assert_string_eq("this value is quoted, quotes are removed",  config_data_find(conf, "section.property2"));
		assert_string_eq("this value is quoted, \" quotes are removed",  config_data_find(conf, "section.property3"));
		assert_string_eq("this value contains =",  config_data_find(conf, "section.property4"));
		assert_string_eq("",  config_data_find(conf, "section.property5"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_property_sectionless_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"property1 = value1\n"
			"property2 = value2\n"
			"[ section ]\n"
			"    property = value\n"
			"[]\n"
			"    property3 = value3\n"
			"    property4 = value4\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_eq_msg(4, conf->entries.len, "unexpected number of sectionless properties");

		str_array_sort(&conf->entries);
		assert_string_eq("property1", str_array_get(&conf->entries, 0));
		assert_string_eq("value1", config_data_find(conf, "property1"));
		assert_string_eq("property2", str_array_get(&conf->entries, 1));
		assert_string_eq("value2", config_data_find(conf, "property2"));
		assert_string_eq("property3", str_array_get(&conf->entries, 2));
		assert_string_eq("value3", config_data_find(conf, "property3"));
		assert_string_eq("property4", str_array_get(&conf->entries, 3));
		assert_string_eq("value4", config_data_find(conf, "property4"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(parse_config_flat_properties_test)
{
	struct config_data *conf;
	const char *config_file_content =
			"section.subsection.property = 1\n"
			"section.new.prop = 2\n"
			"my.section.prop = hello\n";

	int fd;
	TEST_START() {
		config_data_init(&conf);

		fd = write_conf_to_fd(config_file_content, strlen(config_file_content));
		assert_true_msg(fd != -1, "failed to allocate and write to pipe");

		int status = parse_config_fd(conf, fd);
		assert_zero_msg(status, "failed to parse config data");

		assert_eq_msg(2, conf->subsections_len, "unexpected number of subsections");

		struct config_data *tmp = conf->subsections[0];
		assert_string_eq("section", tmp->section);
		assert_eq_msg(2, tmp->subsections_len, "unexpected number of subsections");

		tmp = conf->subsections[1];
		assert_string_eq("my", tmp->section);
		assert_eq_msg(1, tmp->subsections_len, "unexpected number of subsections");

		assert_string_eq("1", config_data_find(conf, "section.subsection.property"));
		assert_string_eq("2", config_data_find(conf, "section.new.prop"));
		assert_string_eq("hello", config_data_find(conf, "my.section.prop"));
	}

	config_data_release(&conf);
	if (fd != -1)
		close(fd);

	TEST_END();
}

TEST_DEFINE(write_config_empty_config_data_test)
{
	struct config_data *conf;
	struct strbuf config_file;

	int fd[2] = {-1, -1};
	TEST_START() {
		config_data_init(&conf);
		strbuf_init(&config_file);

		assert_false_msg(pipe(fd) < 0, "failed to initialize pipe");

		int status = write_config_fd(conf, fd[1]);
		assert_eq_msg(0, status, "failed to allocate and write to pipe");

		close(fd[1]);
		fd[1] = -1;

		read_fd_to_strbuf(fd[0], &config_file);
		assert_string_eq_msg("", config_file.buff, "expected empty file from write_config_fd when config_data is empty");
	}

	strbuf_release(&config_file);
	config_data_release(&conf);
	if (fd[0] != -1)
		close(fd[0]);
	if (fd[1] != -1)
		close(fd[1]);

	TEST_END();
}

TEST_DEFINE(write_config_simple_section_name_format_test)
{
	struct config_data *conf;
	struct strbuf config_file;

	const char *expected_config_file =
			"[ section ]\n"
			"\tproperty = \"my value\"\n"
			"[ section.subsection ]\n"
			"\tproperty = \"my value\"\n";

	int fd[2] = {-1, -1};
	TEST_START() {
		config_data_init(&conf);
		strbuf_init(&config_file);

		config_data_insert(conf, "section.property", "my value");
		config_data_insert(conf, "section.subsection.property", "my value");

		assert_false_msg(pipe(fd) < 0, "failed to initialize pipe");

		int status = write_config_fd(conf, fd[1]);
		assert_eq_msg(0, status, "failed to allocate and write to pipe");

		close(fd[1]);
		fd[1] = -1;

		read_fd_to_strbuf(fd[0], &config_file);
		assert_string_eq_msg(expected_config_file, config_file.buff, "unexpected config file format");
	}

	strbuf_release(&config_file);
	config_data_release(&conf);
	if (fd[0] != -1)
		close(fd[0]);
	if (fd[1] != -1)
		close(fd[1]);

	TEST_END();
}

TEST_DEFINE(write_config_quoted_section_name_format_test)
{
	struct config_data *conf;
	struct strbuf config_file;

	const char *expected_config_file =
			"[ section ]\n"
			"\tproperty = \"my value\"\n"
			"[ section.\"quoted subsection\" ]\n"
			"\tproperty = \"my value\"\n";

	int fd[2] = {-1, -1};
	TEST_START() {
		config_data_init(&conf);
		strbuf_init(&config_file);

		config_data_insert(conf, "section.property", "my value");
		config_data_insert(conf, "\"section\".\"quoted subsection\".property", "my value");

		assert_false_msg(pipe(fd) < 0, "failed to initialize pipe");

		int status = write_config_fd(conf, fd[1]);
		assert_eq_msg(0, status, "failed to allocate and write to pipe");

		close(fd[1]);
		fd[1] = -1;

		read_fd_to_strbuf(fd[0], &config_file);
		assert_string_eq_msg(expected_config_file, config_file.buff, "unexpected config file format");
	}

	strbuf_release(&config_file);
	config_data_release(&conf);
	if (fd[0] != -1)
		close(fd[0]);
	if (fd[1] != -1)
		close(fd[1]);

	TEST_END();
}

TEST_DEFINE(write_config_escaped_chars_section_name_format_test)
{
	struct config_data *conf;
	struct strbuf config_file;

	const char *expected_config_file =
			"[ section ]\n"
			"\tproperty = \"my value\"\n"
			"[ section.\"\\\"subsection\\\"\" ]\n"
			"\tproperty = \"my value\"\n"
			"[ section.\"sub\\\\section\" ]\n"
			"\tproperty = \"my value\"\n";

	int fd[2] = {-1, -1};
	TEST_START() {
		config_data_init(&conf);
		strbuf_init(&config_file);

		config_data_insert(conf, "section.property", "my value");
		config_data_insert(conf, "\"section\".\"\\\"subsection\\\"\".property", "my value");
		config_data_insert(conf, "\"section\".\"sub\\\\sec\\tion\".property", "my value");

		assert_false_msg(pipe(fd) < 0, "failed to initialize pipe");

		int status = write_config_fd(conf, fd[1]);
		assert_eq_msg(0, status, "failed to allocate and write to pipe");

		close(fd[1]);
		fd[1] = -1;

		read_fd_to_strbuf(fd[0], &config_file);
		assert_string_eq_msg(expected_config_file, config_file.buff, "unexpected config file format");
	}

	strbuf_release(&config_file);
	config_data_release(&conf);
	if (fd[0] != -1)
		close(fd[0]);
	if (fd[1] != -1)
		close(fd[1]);

	TEST_END();
}

TEST_DEFINE(write_config_sectionless_properties_unindented_test)
{
	struct config_data *conf;
	struct strbuf config_file;

	const char *expected_config_file =
			"myproperty = \"my value\"\n"
			"\"my second property\" = \"my value\"\n"
			"[ section.subsection ]\n"
			"\tproperty = \"my value\"\n";

	int fd[2] = {-1, -1};
	TEST_START() {
		config_data_init(&conf);
		strbuf_init(&config_file);

		config_data_insert(conf, "myproperty", "my value");
		config_data_insert(conf, "\"my second property\"", "my value");
		config_data_insert(conf, "section.subsection.property", "my value");

		assert_false_msg(pipe(fd) < 0, "failed to initialize pipe");

		int status = write_config_fd(conf, fd[1]);
		assert_eq_msg(0, status, "failed to allocate and write to pipe");

		close(fd[1]);
		fd[1] = -1;

		read_fd_to_strbuf(fd[0], &config_file);
		assert_string_eq_msg(expected_config_file, config_file.buff, "unexpected config file format");
	}

	strbuf_release(&config_file);
	config_data_release(&conf);
	if (fd[0] != -1)
		close(fd[0]);
	if (fd[1] != -1)
		close(fd[1]);

	TEST_END();
}

TEST_DEFINE(write_config_quoted_property_names_test)
{
	struct config_data *conf;
	struct strbuf config_file;

	const char *expected_config_file =
			"[ section.subsection ]\n"
			"\tproperty1 = 123\n"
			"\t\"property 2\" = \"my value\"\n"
			"\t\"property.3\" = \"my value\"\n";

	int fd[2] = {-1, -1};
	TEST_START() {
		config_data_init(&conf);
		strbuf_init(&config_file);

		config_data_insert(conf, "section.subsection.\"property1\"", "123");
		config_data_insert(conf, "section.subsection.\"property 2\"", "my value");
		config_data_insert(conf, "section.subsection.\"property.3\"", "my value");

		assert_false_msg(pipe(fd) < 0, "failed to initialize pipe");

		int status = write_config_fd(conf, fd[1]);
		assert_eq_msg(0, status, "failed to allocate and write to pipe");

		close(fd[1]);
		fd[1] = -1;

		read_fd_to_strbuf(fd[0], &config_file);
		assert_string_eq_msg(expected_config_file, config_file.buff, "unexpected config file format");
	}

	strbuf_release(&config_file);
	config_data_release(&conf);
	if (fd[0] != -1)
		close(fd[0]);
	if (fd[1] != -1)
		close(fd[1]);

	TEST_END();
}

TEST_DEFINE(write_config_quoted_property_value_test)
{
	struct config_data *conf;
	struct strbuf config_file;

	const char *expected_config_file =
			"[ section.subsection ]\n"
			"\tproperty1 = \" property with spaces \"\n"
			"\tproperty2 = \"property.with.symbols\"\n"
			"\tproperty3 = azAZ1234567890_\n";

	int fd[2] = {-1, -1};
	TEST_START() {
		config_data_init(&conf);
		strbuf_init(&config_file);

		config_data_insert(conf, "section.subsection.property1", " property with spaces ");
		config_data_insert(conf, "section.subsection.property2", "property.with.symbols");
		config_data_insert(conf, "section.subsection.property3", "azAZ1234567890_");

		assert_false_msg(pipe(fd) < 0, "failed to initialize pipe");

		int status = write_config_fd(conf, fd[1]);
		assert_eq_msg(0, status, "failed to allocate and write to pipe");

		close(fd[1]);
		fd[1] = -1;

		read_fd_to_strbuf(fd[0], &config_file);
		assert_string_eq_msg(expected_config_file, config_file.buff, "unexpected config file format");
	}

	strbuf_release(&config_file);
	config_data_release(&conf);
	if (fd[0] != -1)
		close(fd[0]);
	if (fd[1] != -1)
		close(fd[1]);

	TEST_END();
}

TEST_DEFINE(write_config_escaped_property_value_test)
{
	struct config_data *conf;
	struct strbuf config_file;

	const char *expected_config_file =
			"[ section.subsection ]\n"
			"\tproperty1 = \" property \\\"with\\\" spaces \"\n"
			"\tproperty2 = \"property\\\\with\\\\symbols\"\n";

	int fd[2] = {-1, -1};
	TEST_START() {
		config_data_init(&conf);
		strbuf_init(&config_file);

		config_data_insert(conf, "section.subsection.property1", " property \"with\" spaces ");
		config_data_insert(conf, "section.subsection.property2", "property\\with\\symbols");

		assert_false_msg(pipe(fd) < 0, "failed to initialize pipe");

		int status = write_config_fd(conf, fd[1]);
		assert_eq_msg(0, status, "failed to allocate and write to pipe");

		close(fd[1]);
		fd[1] = -1;

		read_fd_to_strbuf(fd[0], &config_file);
		assert_string_eq_msg(expected_config_file, config_file.buff, "unexpected config file format");
	}

	strbuf_release(&config_file);
	config_data_release(&conf);
	if (fd[0] != -1)
		close(fd[0]);
	if (fd[1] != -1)
		close(fd[1]);

	TEST_END();
}

TEST_DEFINE(is_config_invalid_invalid_config_test)
{
	TEST_START() {
		assert_eq_msg(-1, is_config_invalid("resources/bad_section_1.config", 0), "expected invalid config");
		assert_eq_msg(-1, is_config_invalid("resources/bad_section_2.config", 0), "expected invalid config");
		assert_eq_msg(-1, is_config_invalid("resources/bad_section_3.config", 0), "expected invalid config");
		assert_eq_msg(-1, is_config_invalid("resources/bad_key_1.config", 0), "expected invalid config");
		assert_eq_msg(-1, is_config_invalid("resources/bad_key_2.config", 0), "expected invalid config");
		assert_eq_msg(-1, is_config_invalid("resources/bad_key_3.config", 0), "expected invalid config");
		assert_eq_msg(-1, is_config_invalid("resources/bad_key_4.config", 0), "expected invalid config");
	}

	TEST_END();
}


TEST_DEFINE(is_config_invalid_valid_config_test)
{
	TEST_START() {
		assert_eq_msg(0, is_config_invalid("resources/good_1.config", 0), "expected valid config");
		assert_eq_msg(0, is_config_invalid("resources/good_2.config", 0), "expected valid config");
		assert_eq_msg(0, is_config_invalid("resources/good_3.config", 0), "expected valid config");
	}

	TEST_END();
}
TEST_DEFINE(is_config_invalid_unrecognized_key_test)
{
	TEST_START() {
		assert_eq_msg(2, is_config_invalid("resources/good_1.config", 1), "expected unrecognized keys");
		assert_eq_msg(2, is_config_invalid("resources/good_2.config", 1), "expected unrecognized keys");
		assert_eq_msg(0, is_config_invalid("resources/good_3.config", 1), "expected recognized keys");
	}

	TEST_END();
}

const char *suite_name = SUITE_NAME;
int test_suite(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "parse_config: empty or blank lines should be ignored when parsing config files", parse_config_ignore_empty_lines_test },
			{ "parse_config: empty files should still parse successfully", parse_config_empty_file_test },
			{ "parse_config: whitespace surrounding section header should be trimmed", parse_config_section_trim_outer_whitespace_test },
			{ "parse_config: whitespace surrounding section names should be trimmed", parse_config_section_trim_inner_whitespace_test },
			{ "parse_config: sections without properties should still parse successfully", parse_config_section_no_properties_test },
			{ "parse_config: properties declared before first section should become entries in root node", parse_config_section_no_section_name_test },
			{ "parse_config: section names should create expected config_data structure", parse_config_section_simple_section_name_test },
			{ "parse_config: quoted section name components should be normalized in config_data structure", parse_config_section_quoted_section_name_test },
			{ "parse_config: character escapes in section names should be normalized in config_data structure", parse_config_section_escaped_chars_section_name_test },
			{ "parse_config: invalid section names should fail to parse", parse_config_section_invalid_name_fail_test },
			{ "parse_config: invalid properties should fail to parse", parse_config_property_invalid_test },
			{ "parse_config: whitespace surrounding property keys or values should be trimmed", parse_config_property_trim_whitespace_test },
			{ "parse_config: quoted property names should parse successfully", parse_config_property_quoted_prop_names_test },
			{ "parse_config: quoted property values should be normalized in config_data structure", parse_config_property_quoted_prop_values_test },
			{ "parse_config: escape sequences in property names should be normalized", parse_config_property_char_escape_prop_names_test },
			{ "parse_config: escape sequences in property values should be normalized", parse_config_property_char_escape_prop_values_test },
			{ "parse_config: sectionless properties should belong to root node", parse_config_property_sectionless_test },
			{ "parse_config: flattened property names should still parse successfully", parse_config_flat_properties_test },

			{ "write_config: empty config_data should write empty file to fd", write_config_empty_config_data_test },
			{ "write_config: simple section names with one or more components should have correct format", write_config_simple_section_name_format_test },
			{ "write_config: section names with one or more quoted components should have correct format", write_config_quoted_section_name_format_test },
			{ "write_config: section names with one or more components with escaped characters should have correct format", write_config_escaped_chars_section_name_format_test },
			{ "write_config: sectionless properties should be unindented", write_config_sectionless_properties_unindented_test },
			{ "write_config: quoted property names should be formatted correctly", write_config_quoted_property_names_test },
			{ "write_config: quoted property values should be formatted correctly", write_config_quoted_property_value_test },
			{ "write_config: property values with special characters should be escaped", write_config_escaped_property_value_test },

			{ "is_config_invalid: invalid config should return -1", is_config_invalid_invalid_config_test },
			{ "is_config_invalid: valid config should return 0", is_config_invalid_valid_config_test },
			{ "is_config_invalid: unrecognized keys should return -2 if unrecognized and 0 if recognized", is_config_invalid_unrecognized_key_test },
			{ NULL, NULL },
	};

	return execute_tests(instance, tests);
}
