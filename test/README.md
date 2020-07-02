# Testing git-chat
Like any good piece of software, git-chat has a suite of unit and integration tests for testing various components. The unit tests are written in C and the integration tests are written in Bash shell script. There are many ways to run git-chat tests, the easiest of which is by using `cmake` and `ctest`.

## Contents
1. [Running Tests](#running-tests)
	1. [Unit](#unit)
		1. [Running Unit Tests with Valgrind Memcheck](#running-unit-tests-with-valgrind-memcheck)
		2. [Configuring Unit Test Execution](#configuring-unit-test-execution)
	2. [Integration](#integration)
		1. [CMake Build Target](#cmake-build-target)
		2. [Integration Runner](#integration-runner)
2. [Writing Tests](#writing-tests)
	1. [Unit Tests](#unit-tests)
		1. [Adding a New Unit Test](#adding-a-new-unit-test)
		2. [Adding a New Test Suite](#adding-a-new-test-suite)
		3. [Assertion Macros](#assertion-macros)
	2. [Integration Tests](#integration-tests)
		1. [Assertion Functions](#assertion-functions)
		2. [Helper Functions](#helper-functions)

## Running Tests
### Unit
Running unit tests is quite straightforward. First, you'll need to build git-chat.
```
$ mkdir build
$ cd build
$ cmake ..
$ make git-chat-unit-tests
```

Next, run the unit tests using any of the methods below:
```
$ ctest -R integration-tests --verbose
$ ./git-chat-unit-tests
```

#### Running Unit Tests with Valgrind Memcheck
To run unit tests with Valgrind memcheck:
```
$ valgrind --tool=memcheck --gen-suppressions=all --leak-check=full \
		--leak-resolution=high --track-origins=yes --vgdb=no --error-exitcode=1 \
		./git-chat-unit-tests
```

#### Configuring Unit Test Execution
The `git-chat-unit-tests` executable accepts a number of environment variables that allow you to configure how the unit tests are executed.
- `GIT_CHAT_TEST_VERBOSE`: Make the unit test output more verbose. This will print description messages for every unit test.
- `GIT_CHAT_TEST_IMMEDIATE`: Exit after the first test failure.

```
$ GIT_CHAT_TEST_VERBOSE=1 ./git-chat-unit-tests
*** str-array ***
[19:46:08] str-array should initialize correctly ........................................................... ok
[19:46:08] str-array should grow in size upon request ...................................................... ok
[19:46:08] str-array should release correctly .............................................................. ok
[19:46:08] accessing element from str-array should return correct element .................................. ok

[...]

[19:46:08] is_valid_argument should differentiate valid and invalid boolean args ........................... ok
[19:46:08] is_valid_argument should differentiate valid and invalid int args ............................... ok
[19:46:08] is_valid_argument should differentiate valid and invalid string args ............................ ok
[19:46:08] is_valid_argument should differentiate valid and invalid command args ........................... ok
*** run-command ***
[19:46:08] Executing a child process from a given directory should correctly chdir to that directory ....... ok
[19:46:08] Executing a child process by providing a full path to the executable should correctly find the executable to run  ok
[19:46:08] Executing a child process by providing an executable that exists on the path should correctly find the executable to run  ok
[19:46:08] Executing a child process with a custom environment should correctly merge the environment with the parent process's environment  ok
[19:46:08] Executing a git command should correctly invoke the git executable .............................. ok
[19:46:08] run_command() should return the exit status code of the child process that was run .............. ok
[19:46:08] Capturing stdout from a child process should correctly build the process output to a string buffer  ok


Test Execution Summary:
Executed: 57
Passed: 57
Failed: 0
```

```
$ GIT_CHAT_TEST_IMMEDIATE=1 ./git-chat-unit-tests
[19:44:45] str-array ....................................................................................... ok
[19:44:45] argv-array ...................................................................................... ok
Assertion failed: strbuf-test.c:206 in strbuf_trim_test()
	Expected equal values, but actual values not equal.

[19:44:45] strbuf .......................................................................................... fail


Test Execution Summary:
Executed: 28
Passed: 27
Failed: 1
```

### Integration
Configuring and running integration tests is a bit more involved, since an installation of git-chat is needed. The steps below should be enough to get you all set up.

#### Using the CMake Build Target
The integration tests are configured as a CMake build target, and can executed by running the following from the project root:
```
$ mkdir build
$ cd build
$ cmake ..

$ # for local installation
$ TEST_GIT_CHAT_INSTALLED=~/bin cmake --build . --target git-chat-integration-tests

$ # or for global installation
$ cmake --build . --target git-chat-integration-tests
```

This is the easiest way to run the tests.

#### Using the Integration Runner
The integration tests can also be executed using the `integration-runner.sh`, located in `test/`.
```
$ mkdir build
$ cd build
$ cmake ..
$ cd test

$ # for local installation
$ ./integration-runner.sh --git-chat-installed ~/bin

$ # or for global installation
$ ./integration-runner.sh
```

The runner recognizes a number of command line arguments and environment variables, which are documented fully in `integration-runner.sh`.

## Writing Tests
### Unit Tests
The unit tests use a lightweight and simple test framework with an unobtrusive syntax and simple suite runner. There are a few steps that need to be taken when writing new unit tests, which are outlined below.

#### Adding a New Unit Test
To add a new unit test to an existing test suite located in `test/unit/`:
1. First create the test:
```
TEST_DEFINE(test_name)
{
	//Put any test setup here

	TEST_START() {
		//put your test code here
	}

	//Put any test teardown here

	TEST_END();
}
```

2. Add the test function to the `struct unit_test test[]` in the suite entry point, being sure that the last entry is `{ NULL, NULL }`. For example:
```
int str_array_test(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "str-array should initialize correctly", str_array_init_test },
			{ "a description for my new test that I just added", test_name },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
```

#### Adding a New Test Suite
1. First create the suite in `test/unit/`. You can use the template below as a starting point:
```
#include "test-lib.h"

TEST_DEFINE(my_test)
{
	int zero = 0;

	TEST_START() {
		assert_zero(zero);
	}

	TEST_END();
}

int my_test_suite(struct test_runner_instance *instance)
{
	struct unit_test tests[] = {
			{ "my first test!", my_test },
			{ NULL, NULL }
	};

	return execute_tests(instance, tests);
}
```

As you add new unit tests, you will need to update your runner function to run those tests.

2. Next, add a function prototype declaration in `test/include/test-suite.h` pointing to the entry of your tests.
```
extern int my_test_suite(struct test_runner_instance *instance);
```

3. Lastly, add the test suite to the unit test runner `test/runner.c`:
```
static struct suite_test tests[] = {
		{ "str-array", str_array_test },
		{ "my new test", my_test_suite },
		{ NULL, NULL }
};

[...]
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
New integration tests should be created in the `test/integration` directory. The new tests must be executable.

All integration tests must first source the test library file, which sets up assertion functions and configures the test environment.

Here is a simple example:
```
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

Tests are executed within a _trash_ directory, which is recreated after every integration suite. So, it is perfectly fine to create files and git repositories, granted that care is taken to avoid changing the current directory.

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

2. `assert_failure`
```
assert_failure (description, [test_setup], test)
Evaluate a command or script and assert that it fails with a non-zero exit status.

Examples:
assert_failure 'integration tests must not be executed from within a git working tree' '
	git rev-parse --is-inside-work-tree
'

assert_failure 'git-chat should not run extension on path if not executable' '
	cat >git-chat-noex <<-\EOF &&
	#!/bin/sh
	echo hello world
	exit 0
	EOF
	chmod -x git-chat-noex
' '
	PATH=$PATH:$(pwd) git chat noex
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