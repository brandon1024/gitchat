# Testing git-chat

Like any good piece of software, git-chat has a suite of unit and integration
tests for testing various components. The unit tests are written in C and the
integration tests are written in Bash. There are many ways to run git-chat
tests, the easiest of which is by running the `test` and `integration` make
targets.

## Contents

1. [Running Tests](#running-tests)
	1. [Unit](#unit)
		1. [Running Unit Tests with Valgrind Memcheck](#running-unit-tests-with-valgrind-memcheck)
		2. [Configuring Unit Test Execution](#configuring-unit-test-execution)
	2. [Integration](#integration)
		1. [Using the CMake Build Target](#using-the-cmake-build-target)
		2. [Using the Integration Runner](#using-the-integration-runner)
2. [Writing Tests](#writing-tests)
	1. [Unit Tests](#unit-tests)
		1. [Adding a New Unit Test Suite](#adding-a-new-unit-test-suite)
		3. [Assertion Macros](#assertion-macros)
	2. [Integration Tests](#integration-tests)
		1. [Assertion Functions](#assertion-functions)
		2. [Helper Functions](#helper-functions)

## Running Tests

### Unit

Running unit tests is quite straightforward. First, you'll need to build
git-chat. Then you can run the `test` target:

```
$ cmake -B build/ -S .
$ make -C build/ all test
```

#### Running Unit Tests with Valgrind Memcheck

Running unit tests under Valgrind memcheck is easy too!

```
$ cmake -B build/ -S .
$ make -C build/ all
$ make -C build/ test ARGS='-T memcheck -V'
```

#### Configuring Unit Test Execution

The unit test runner accepts the `GIT_CHAT_TEST_IMMEDIATE` environment variable,
which is used to configure how the unit tests behave when a test fails. When
set, the test suite will stop immediately when a failure is encountered.
Otherwise, all tests are executed.

### Integration

The steps for running integration tests are similar to running unit tests,
except that you need to install git-chat first.

#### Using the CMake Build Target

The integration tests are configured as a CMake build target, and can executed
as follows:

```
$ cmake -B build/ -S .
$ make -C build/ all install
$ make -C build/ integration
```

This is the easiest way to run the tests.

#### Using the Integration Runner

The integration tests can also be executed using the `integration-runner.sh`,
located in `test/`. This is a more advanced way of running tests.

```
$ cmake -B build/ -S .
$ make -C build/ all install

$ # for local installation
$ ./test/integration-runner.sh --from-dir build/test --git-chat-installed ~/bin
$ # or for global installation
$ ./test/integration-runner.sh --from-dir build/test
```

See `integration-runner.sh --help` for more information.

## Writing Tests

### Unit Tests

The unit tests use a lightweight and simple test framework with an unobtrusive
syntax and simple suite runner. There are a few steps that need to be taken when
writing new unit tests, which are outlined below.

#### Adding a New Unit Test Suite

1. First create the test suite under `test/unit/` and add tests:

```c
#include "test-lib.h"

TEST_DEFINE(test_name)
{
	// put any test setup here

	TEST_START() {
		// put your test code here
	}

	// put any test teardown here

	TEST_END();
}
```

2. To allow this test to get picked up by the test runner, add an entrypoint and test name:

```c
const char *suite_name = SUITE_NAME;
int test_suite(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "str-array should initialize correctly", str_array_init_test },
			{ "a description for my new test that I just added", test_name },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
```

3. Lastly, register this test suite to get run by adding the test to `test/CMakeLists.txt`:

```c
add_unit_test(${TEST_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/unit/${TEST_SOURCE_FILE})
```

#### Assertion Macros

Default Assertion Macros:

- assert_string_eq(expected, actual)
- assert_string_neq(expected, actual)
- assert_eq(expected, actual)
- assert_neq(expected, actual)
- assert_true(actual)
- assert_false(actual)
- assert_null(actual)
- assert_nonnull(actual)
- assert_zero(actual)
- assert_nonzero(actual)

Custom Assertion Message Macros:

- assert_string_eq_msg(expected, actual, fmt, ...)
- assert_string_neq_msg(expected, actual, fmt, ...)
- assert_eq_msg(expected, actual, fmt, ...)
- assert_neq_msg(expected, actual, fmt, ...)
- assert_true_msg(actual, fmt, ...)
- assert_false_msg(actual, fmt, ...)
- assert_null_msg(actual, fmt, ...)
- assert_nonnull_msg(actual, fmt, ...)
- assert_zero_msg(actual, fmt, ...)
- assert_nonzero_msg(actual, fmt, ...)

### Integration Tests

New integration tests should be created in the `test/integration` directory. The
new tests must be executable.

All integration tests must first source the test library file, which sets up
assertion functions and configures the test environment.

Here is a simple example:

```shell
#!/usr/bin/env bash

# All integration tests must source the test library
source ./test-lib.sh

assert_success '<test description>' '
	test 1 = 1
'

assert_success '<test description>' '
	echo 'setup test prerequisites' >file
' '
	cat file | grep 'setup'
'
```

Tests may use any standard bash commands or builtins, such as `grep` or `sed`.

Tests are executed within a _trash_ directory, which is recreated after every
integration suite. So, it is perfectly fine to create files and git
repositories, granted that care is taken to avoid changing the current
directory.

#### Assertion Functions

1. `assert_success`

```
assert_success (description, [test_setup], test)
Evaluate a command or script and assert that it can execute and return with a zero exit status.

Example:
assert_success 'git-chat invoked as a git extension without arguments should exit with status 0' '
	git chat
'

assert_success '<test description>' '
	echo 'setup test prerequisites' >file
' '
	cat file | grep 'setup'
'
```

#### Helper Functions

1. reset_trash_dir

```
reset_trash_dir ()
Remove all files and directories in the trash dir.

Example:
assert success 'test reset_trash_dir' '
	echo test >out &&
	reset_trash_dir &&
	[ -z "$(ls -A .)" ]
'
```

2. setup_test_gpg

```
setup_test_gpg ([private key path])
Configure environment to use GPG. For commands that use GPG, this is necessary
to avoid clobbering the local gpg configuration (trustdb, keyring, etc).

This utility will:
- create trustdb and keybox in ${TEST_TRASH_DIR}/.gpg_tmp
- import public and private key for test user

If an argument is given, imports private key from the given path. Otherwise, uses
test user private key from ${TEST_RESOURCES_DIR}/gpgkeys/test_user.gpg

Example:
assert_success 'git chat message without any recipients or keys should fail' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg
' '
	! git chat message -m "hello world" 2>err &&
	grep "no gpg keys found" err
'
```
