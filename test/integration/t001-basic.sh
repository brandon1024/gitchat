#!/usr/bin/env bash

# Ensure the test-lib can be found (not needed in further tests)
if [ ! -f test-lib.sh ]; then
	echo 'Could not find test-lib.sh' 1>&2
	exit 1
fi

source ./test-lib.sh

#
# Verify Installation
#
assert_success 'integration tests must not be executed from within a git working tree' '
	! git rev-parse --is-inside-work-tree
'

assert_success 'git-chat should install on the system PATH' '
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
