#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git chat message without any recipients or keys should fail' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	[ -z "$(ls -A .keys/)" ]
' '
	! git chat message -m "hello world" 2>err &&
	grep "no message recipients" err
'

assert_success 'git chat message with unknown recipient should fail' '
	reset_trash_dir &&
	git chat init &&
	cp $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg .keys &&
	setup_test_gpg
' '
	! git chat message -m "hello world" --recipient unknown@unknown.ca 2>err &&
	grep "one or more recipients have no known GPG keys in the keyring" err
'

assert_success 'author of encrypted message should not be able to decrypt message if not specified as recipient' '
	reset_trash_dir &&
	git chat init &&
	cp $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg .keys &&
	setup_test_gpg
' '
	git chat message -m "hello world" --recipient alice.jones@example.com &&
	git show -s --format="%B" HEAD >commit_msg &&
	! gpg2 --no-default-keyring --keyring "$(pwd)/.git/chat-cache/keyring.gpg" \
		--batch --passphrase password --decrypt commit_msg 2>err
	grep "No secret key" err
'

assert_success 'author of encrypted message must be able to decrypt message if included as recipient' '
	reset_trash_dir &&
	git chat init &&
	cp $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg .keys &&
	setup_test_gpg
' '
	git chat message -m "hello world" &&
	git show -s --format="%B" HEAD >commit_msg &&
	echo password | gpg2 --pinentry-mode=loopback --passphrase-fd 0 --batch --decrypt commit_msg >out &&
	grep "hello world" out
'

