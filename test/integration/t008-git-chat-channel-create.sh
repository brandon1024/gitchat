#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git-chat channel create -h should show usage information' '
	git chat channel create -h &&
	git chat channel create --help >out &&
	grep '\''^usage: git chat channel create'\'' out
'

assert_success 'git-chat channel create should update config with channel details' '
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world"
''
	git chat channel create --name "test-channel" --description "test channel" testing &&
	git chat config --get "channel.testing.createdby" >createdby &&
	grep "test.user@testing.com" createdby &&
	git chat config --get "channel.testing.name" >name &&
	grep "test-channel" name &&
	git chat config --get "channel.testing.description" >desc &&
	grep "test channel" desc
'

assert_success 'git-chat channel create without alias should use refname' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world"
''
	git chat channel create --description "test channel" testing &&
	git chat config --get "channel.testing.name" >name &&
	grep "testing" name
'

assert_success 'git-chat channel create without description should leave description property unset' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world"
''
	git chat channel create testing &&
	! git chat config --get "channel.testing.description"
'

assert_success 'git-chat channel create should commit config file changes' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world"
''
	git chat channel create -n "test-channel" -d "test channel" testing &&
	git show -s --format="%B" HEAD >commit_msg &&
	grep "You have reached the beginning of channel '\''test-channel'\''." commit_msg
'
