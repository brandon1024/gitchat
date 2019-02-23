#ifndef GIT_CHAT_TEST_LIB_H
#define GIT_CHAT_TEST_LIB_H

/**
 * test-lib API
 *
 * The test-lib API is a short and simple unit test framework, with an
 * unobtrusive syntax and simple suite runner. To use the test-lib api,
 * simply include this header file.
 *
 * * Creating a Unit Test:
 * The structure for a new test is simple. An example can be found below:
 * ```
 * #include "test-lib.h"
 *
 * int component_test(struct test_runner_instance *instance)
 * {
 * 		struct unit_test tests[] = {
 * 			{ NULL, NULL }
 * 		};
 *
 * 		return execute_tests(instance, tests);
 * }
 * ```
 *
 * First, define a "runner" function which accepts a struct test_runner_instance
 * and returns int. The runner shown above will execute every test in the array
 * `tests` until { NULL, NULL }. In ths example, no tests are executed.
 *
 * Next, you need to define your unit tests.
 * ```
 * TEST_DEFINE(unit_test_name)
 * {
 * 		//setup test here
 *
 * 		TEST_START() {
 *
 * 		}
 *
 * 		//teardown test here
 * 		TEST_END();
 * }
 * ```
 *
 * As you add new unit tests, you will need to update your runner function to run
 * those tests. For example, the runner for the unit test above would look like this:
 * ```
 * int component_test(struct test_runner_instance instance)
 * {
 * 		struct unit_test tests[] = {
 * 			{ "component should do this thing correctly", unit_test_name },
 * 			{ NULL, NULL }
 * 		};
 *
 * 		return execute_tests(instance, tests);
 * }
 * ```
 *
 * Adding New Tests:
 * To add a new test to the suite:
 * - first, add a function prototype declaration in `test-suite.h` pointing to
 * the entry of your tests.
 * - update the `runner.c` tests array with your new test.
 *
 *
 * Assertion Macros:
 * The various assertion macros are listed below. If an assertion fails, control
 * will exit the TEST_START() block, setting the appropriate internal state variables
 * which will exit the function appropriately at the end, once the teardown is
 * complete.
 *
 * Default Assertion Macros:
 * - assert_string_eq(a, b)
 * - assert_string_neq(a, b)
 * - assert_eq(a, b)
 * - assert_neq(a, b)
 * - assert_true(a)
 * - assert_false(a)
 * - assert_null(a)
 * - assert_nonnull(a)
 * - assert_zero(a)
 * - assert_nonzero(a)
 *
 * Custom Assertion Message Macros:
 * - assert_string_eq_msg(a, b, m, ...)
 * - assert_string_neq_msg(a, b, m, ...)
 * - assert_eq_msg(a, b, m, ...)
 * - assert_neq_msg(a, b, m, ...)
 * - assert_true_msg(a, m, ...)
 * - assert_false_msg(a, m, ...)
 * - assert_null_msg(a, m, ...)
 * - assert_nonnull_msg(a, m, ...)
 * - assert_zero_msg(a, m, ...)
 * - assert_nonzero_msg(a, m, ...)
 *
 * Custom assertion message macros will pass m and any further argument to
 * printf. Refer to `man printf` for a full list of supported format message
 * flags and specifiers.
 * */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TEST_DEFINE(__test_name) static int __test_name ()
#define TEST_START() int __ret = 0; for(int __i = 1;__i--;)
#define TEST_END() return __ret

#define assert_string_eq(a, b) \
	if((a) && !(b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected '%s' but got NULL.", (a)); \
		__ret = 1; \
		break; \
	} else if(!(a) && (b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected NULL but got '%s'.", (b)); \
		__ret = 1; \
		break; \
	} else if(strcmp((a), (b)) != 0) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected '%s' but got '%s'.", (a), (b)); \
		__ret = 1; \
		break; \
	}

#define assert_string_eq_msg(a, b, ...) \
	if((a) && !(b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	} else if(!(a) && (b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	} else if(strcmp((a), (b)) != 0) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

#define assert_string_neq(a, b) \
	if(((a) && !(b)) || (!(a) && (b))) {} \
	else if(!(a) && !(b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected different strings, but both were NULL.", (a)); \
		__ret = 1; \
		break; \
	} else if(a == b || !strcmp((a), (b))) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected '%s' but got '%s'.", (a), (b)); \
		__ret = 1; \
		break; \
	}

#define assert_string_neq_msg(a, b, ...) \
	if(((a) && !(b)) || (!(a) && (b))) {} \
	else if(!(a) && !(b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected different strings, but both were NULL.", (a)); \
		__ret = 1; \
		break; \
	} else if(a == b || !strcmp((a), (b))) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

#define assert_eq(a, b) \
	if((a) != (b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected equal values, but actual values not equal."); \
		__ret = 1; \
		break; \
	}

#define assert_eq_msg(a, b, ...) \
	if((a) != (b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

#define assert_neq(a, b) \
	if((a) == (b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected non-equal values, but actual values equal."); \
		__ret = 1; \
		break; \
	}

#define assert_neq_msg(a, b, ...) \
	if((a) == (b)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

#define assert_true(a) \
	if(!(a)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected true, but got false."); \
		__ret = 1; \
		break; \
	}

#define assert_true_msg(a, ...) \
	if(!(a)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

#define assert_false(a) \
	if(!!(a)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected false, but got true."); \
		__ret = 1; \
		break; \
	}

#define assert_false_msg(a, ...) \
	if(!!(a)) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

#define assert_null(a) \
	if((a) != NULL) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected null, but got non-null."); \
		__ret = 1; \
		break; \
	}

#define assert_null_msg(a, ...) \
	if((a) != NULL) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

#define assert_nonnull(a) \
	if((a) == NULL) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected non-null, but got null."); \
		__ret = 1; \
		break; \
	}

#define assert_nonnull_msg(a, ...) \
	if((a) == NULL) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

#define assert_zero(a) \
	if((a) != 0) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected zero, but got non-zero."); \
		__ret = 1; \
		break; \
	}

#define assert_zero_msg(a, ...) \
	if((a) != 0) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

#define assert_nonzero(a) \
	if((a) == 0) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, "Expected non-zero, but got zero."); \
		__ret = 1; \
		break; \
	}

#define assert_nonzero_msg(a, ...) \
	if((a) == 0) { \
		print_assertion_failure_message(__FILE__, __LINE__, __func__, __VA_ARGS__); \
		__ret = 1; \
		break; \
	}

struct test_runner_instance;

struct suite_test {
	const char *test_name;
	int (*fn)(struct test_runner_instance *);
};

struct unit_test {
	const char *test_name;
	int (*fn)();
};

int execute_suite(struct suite_test tests[], int verbose, int immediate);
int execute_tests(struct test_runner_instance *instance, struct unit_test *tests);
void print_assertion_failure_message(const char *file_path, int line_number,
		const char *func_name, const char *fmt, ...);

#endif //GIT_CHAT_TEST_LIB_H
