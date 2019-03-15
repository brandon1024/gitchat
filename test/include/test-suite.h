#ifndef GIT_CHAT_SUITE_H
#define GIT_CHAT_SUITE_H

extern int str_array_test(struct test_runner_instance *instance);
extern int argv_array_test(struct test_runner_instance *instance);
extern int strbuf_test(struct test_runner_instance *instance);
extern int parse_config_test(struct test_runner_instance *instance);
extern int usage_test(struct test_runner_instance *instance);
extern int run_command_test(struct test_runner_instance *instance);

#endif //GIT_CHAT_SUITE_H
