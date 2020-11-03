#ifndef GIT_CHAT_SUITE_H
#define GIT_CHAT_SUITE_H

extern int str_array_test(struct test_runner_instance *);
extern int argv_array_test(struct test_runner_instance *);
extern int strbuf_test(struct test_runner_instance *);
extern int run_command_test(struct test_runner_instance *);
extern int fs_utils_test(struct test_runner_instance *);
extern int parse_options_test(struct test_runner_instance *);
extern int config_data_test(struct test_runner_instance *);
extern int config_key_test(struct test_runner_instance *);
extern int config_defaults_test(struct test_runner_instance *);
extern int node_visitor_test(struct test_runner_instance *);
extern int parse_config_test(struct test_runner_instance *);
extern int git_commit_parse_test(struct test_runner_instance *);

#endif //GIT_CHAT_SUITE_H
