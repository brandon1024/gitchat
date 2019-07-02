#!/usr/bin/env bash

. ./test-lib.sh

# encrypt with one key, decrypt with the other
assert_success 'git chat message -m should encrypt the message given at the command line' '
	reset_trash_dir &&
	git chat init &&
	cp $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg .keys
' '
	echo hi
'

