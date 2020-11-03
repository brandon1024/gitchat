#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git-chat channel list -h should show usage information' '
	git chat channel list -h &&
	git chat channel list --help >out &&
	grep '\''^usage: git chat channel list'\'' out
'

assert_success 'git-chat channel list should display all local channels' '
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world"
''
	git chat channel create --name "test-channel" --description "test channel" testing &&
	git chat channel switch master &&
	git chat channel create --name "development" --description "dev stuff" devel &&
	git chat channel switch master &&
	git chat channel create --name "rnd" --description "urandom" random &&
	git chat channel list >out &&
	grep "local channels" out &&
	! grep "remote channels" out &&
	grep -E "[[:space:]]+devel.*development.*dev stuff" out &&
	grep -E "[[:space:]]+master.*master.*General Channel" out &&
	grep -E "random.*rnd.*urandom" out &&
	grep -E "[[:space:]]+testing.*test-channel.*test channel" out
'

# displays remote channels
assert_success 'git-chat channel list should display all remote channels channels with -a flag' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world" &&

	# create pseudo-remote
	mkdir upstream.git &&
	pushd upstream.git &&
	git init --bare
	popd &&
	mkdir next.git &&
	pushd next.git &&
	git init --bare
	popd &&

	git remote add upstream "$(pwd)/upstream.git" &&
	git remote add next "$(pwd)/next.git"
''
	git chat channel create --name "test-channel" --description "test channel" testing &&
	git push upstream testing &&
	git chat channel switch master &&

	git chat channel create --name "development" --description "dev stuff" devel &&
	git push next devel &&
	git chat channel switch master &&

	git chat channel list -a >out &&

	grep -A1 "remote channels \[upstream\]" out >channel &&
	cat channel | tail -1 >channel.testing &&
	grep -E "testing.*test-channel.*test channel" channel.testing &&

	grep -A1 "remote channels \[next\]" out >channel &&
	cat channel | tail -n 1 >channel.devel &&
	grep -E "devel.*development.*dev stuff" channel.devel
'

# displays correct count
assert_success 'git-chat channel list should correctly calculate message count' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world" &&
''
	git chat channel create --name "test-channel" --description "test channel" testing &&
	git chat message -m "hello world" &&
	git chat channel switch master &&

	git chat channel create --name "development" --description "dev stuff" devel &&
	git chat message -m "hello world" &&
	git chat message -m "hello world" &&
	git chat message -m "hello world" &&

	git chat channel list >out &&

	# master should have 3 messages
	grep "master.*[3]" out &&

	# testing should have 5 messages
	grep "testing.*[5]" out &&

	# devel should have 7 messages
	grep "devel.*[7]" out
'

# filters uninteresting
assert_success 'git-chat channel list should filter upstream branches up to date with local counterpart' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg &&
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	git chat message -m "hello world" &&

	# create pseudo-remote
	mkdir upstream.git &&
	pushd upstream.git &&
	git init --bare
	popd &&

	git remote add upstream "$(pwd)/upstream.git"
''
	git chat channel create --name "test-channel" --description "test channel" testing &&
	git push upstream testing &&
	git chat channel switch master &&

	git chat channel create --name "development" --description "dev stuff" devel &&
	git push upstream devel &&
	git chat channel switch master &&

	git chat channel list >out &&

	! grep "remote channels \[upstream\]" out &&
	! grep -E "testing.*test-channel.*test channel" channel.testing &&
	! grep -E "devel.*development.*dev stuff" channel.devel
'
