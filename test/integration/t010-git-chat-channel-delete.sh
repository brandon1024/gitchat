#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git-chat channel delete -h should show usage information' '
	git chat channel delete -h &&
	git chat channel delete --help >out &&
	grep '\''^usage: git chat channel delete'\'' out
'

assert_success 'git-chat channel delete should delete a local channel' '
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
	grep "master" current_branch &&

	git chat channel delete testing &&
	! git rev-parse --abbrev-ref testing
'

assert_success 'git-chat channel delete with unknown ref should fail' '
	reset_trash_dir
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world"
''
	! git chat channel delete testing 2>err &&
	grep "couldn'\''t delete channel '\''testing'\''" err
'

assert_success 'git-chat channel delete should fail if branch is checked out' '
	reset_trash_dir
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world"
''
	git chat channel create --name "test-channel" --description "test channel" testing &&
	git rev-parse --abbrev-ref HEAD >current_branch &&
	grep "testing" current_branch &&

	! git chat channel delete testing 2>err &&
	grep "couldn'\''t delete channel '\''testing'\''" err
'
