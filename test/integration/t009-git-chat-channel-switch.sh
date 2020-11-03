#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git-chat channel switch -h should show usage information' '
	git chat channel switch -h &&
	git chat channel switch --help >out &&
	grep '\''^usage: git chat channel switch'\'' out
'

assert_success 'git-chat channel switch should switch to another channel by reference' '
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world"
''
	git chat channel create --name "test-channel" --description "test channel" testing &&
	git rev-parse --abbrev-ref HEAD >current_branch &&
	grep "testing" current_branch &&
	git chat channel switch master &&
	git rev-parse --abbrev-ref HEAD >current_branch &&
	grep "master" current_branch
'

assert_success 'git-chat channel switch should exit with appropriate status if branch does not exist' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world"
''
	git chat channel create --name "test-channel" --description "test channel" testing &&
	git rev-parse --abbrev-ref HEAD >current_branch &&
	grep "testing" current_branch &&
	! git chat channel switch testing2 2>err &&
	grep "couldn'\''t switch to channel '\''testing2'\''" err &&
	grep "does a channel with that refname exist?" err
'
