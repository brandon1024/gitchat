#!/bin/sh

# Ensure the test-lib can be found (not needed in further tests)
if [ ! -f test-lib.sh ]; then
     echo 'Could not find test-lib.sh' 1>&2
     exit 1
fi

. ./test-lib.sh

echo '*** git-chat without arguments should exit with status 0 ***'
assert_zero_exit 'git chat'
assert_zero_exit 'git-chat'

echo '*** git-chat -h should exit with status 0 ***'
assert_zero_exit 'git chat -h'

echo '*** git-chat -v should exit with status 0 ***'
assert_zero_exit 'git chat -v'

echo '*** git-chat --version should exit with status 0 ***'
assert_zero_exit 'git chat --version'
