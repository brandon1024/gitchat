#!/usr/bin/env bash

# Ensure the test-lib can be found (not needed in further tests)
if [ ! -f test-lib.sh ]; then
	echo 'Could not find test-lib.sh' 1>&2
	exit 1
fi

source ./test-lib.sh

#
# Self Test: Verify Assertion Functions
#
assert_success 'reset_trash_dir should correctly clear trash directory contents' '
	echo test >out &&
	reset_trash_dir &&
	[ -z "$(ls -A .)" ]
'

#
# Verify Installation
#
assert_success 'integration tests must not be executed from within a git working tree' '
	! git rev-parse --is-inside-work-tree
'

assert_success 'git-chat should exist on the PATH' '
	which git-chat
'

#
# Basic git-chat Assertions
#
assert_success 'git-chat invoked without arguments should exit with status 0' '
	git-chat | grep '\''^usage: git chat'\''
'

assert_success 'git-chat invoked as a git extension without arguments should exit with status 0' '
	git chat
'

assert_success 'git chat -h should exit with status 0' '
	git-chat -h | grep '\''^usage: git chat'\''
'

assert_success 'git chat with unknown flag should exit with status 1 and print useful message' '
	git chat --test 2>err
	test "$?" = "1" &&
	grep "error: unknown command or option '\''--test'\''" err
'

assert_success 'git chat -v should exit with status 0' '
	git-chat -v > output &&
	grep "^git-chat version" output &&
	grep "^git version" output
'

assert_success 'git chat --version should exit with status 0' '
	git-chat --version > output &&
	grep "^git-chat version" output &&
	grep "^git version" output
'
