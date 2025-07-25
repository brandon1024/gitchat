#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git chat message -h should display usage info' '
	git chat message -h &&
	git chat message --help >out &&
	grep '\''^usage: git chat message'\'' out
'

assert_success 'git chat message without any recipients or keys should fail' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg
' '
	! git chat message -m "hello world" 2>err &&
	grep "no message recipients" err
'

assert_success 'git chat message with unknown recipient should fail' '
	setup_test_gpg &&
	git chat import-key -f "$TEST_RESOURCES_DIR/gpgkeys/ajones_noexpire.pub.gpg"
' '
	git chat message -m "hello world" --recipient alice.jones@example.com &&
	! git chat message -m "hello world" --recipient unknown@unknown.ca 2>err &&
	grep "one or more message recipients have no public gpg key available" err
'

assert_success 'author of encrypted message should not be able to decrypt message if not specified as recipient' '
	setup_test_gpg
' '
	git chat message -m "hello world" --recipient alice.jones@example.com &&
	git show -s --format="%B" HEAD >commit_msg &&
	! gpg2 --no-default-keyring --keyring "$(pwd)/.git/.gnupg/keyring.gpg" \
		--batch --passphrase password --decrypt commit_msg 2>err
	grep "No secret key" err
'

assert_success 'author of encrypted message must be able to decrypt message if included as recipient' '
	setup_test_gpg &&
	git chat import-key -f "$TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg"
' '
	git chat message -m "hello world" &&
	git show -s --format="%B" HEAD >commit_msg &&
	echo password | gpg2 --pinentry-mode=loopback --passphrase-fd 0 --batch --decrypt commit_msg >out &&
	grep "hello world" out
'

assert_success 'message given as file should correctly read from file' '
	setup_test_gpg
' '
	echo "hello world2" > msg &&
	git chat message -f msg &&
	git show -s --format="%B" HEAD >commit_msg &&
	echo password | gpg2 --pinentry-mode=loopback --passphrase-fd 0 --batch --decrypt commit_msg >out &&
	grep "hello world2" out
'

assert_success 'message supplied via stdin should correctly encrypt message' '
	setup_test_gpg
' '
	echo "hello world3" | git chat message -f - &&
	git show -s --format="%B" HEAD >commit_msg &&
	echo password | gpg2 --pinentry-mode=loopback --passphrase-fd 0 --batch --decrypt commit_msg >out &&
	grep "hello world3" out
'

assert_success 'mixing --message and --file should fail (unsupported)' '
	setup_test_gpg
' '
	! git chat message --message "test" --file "unknown" 2>err &&
	grep "mixing \-\-message and \-\-file is not supported" err
'

assert_success 'missing argument to --message should fail' '
	setup_test_gpg
' '
	! git chat message -m 2>err &&
	grep "error: unknown option" err &&
	! git chat message --message 2>err &&
	grep "error: unknown option" err
'

assert_success 'missing argument to --file should fail' '
	setup_test_gpg
' '
	! git chat message -f 2>err &&
	grep "error: unknown option" err &&
	! git chat message --file 2>err &&
	grep "error: unknown option" err
'

assert_success 'passing inexistent file to --file should fail' '
	setup_test_gpg
' '
	! git chat message --file unknownfile 2>err &&
	grep "failed to open file '\''unknownfile'\''" err
'

assert_success 'existence of a .trusted-keys file should filter recipients' '
	setup_test_gpg
' '
	echo "49A9FED4003D28CB5D8CF96748F6785D011F494B" >.git/.trusted-keys &&
	echo "41C4155B702593BFF6D7D140AB4F26E5A765DFDC" >>.git/.trusted-keys &&
	git chat message -m "hello world" &&
	git show -s --format="%B" HEAD >commit_msg &&
	echo password | gpg2 --pinentry-mode=loopback --passphrase-fd 0 --batch --decrypt commit_msg >out &&
	grep "hello world" out &&
	gpg2 --list-only --list-packets <commit_msg >message_details 2>&1 &&
	grep "C5E184648F6CEA47" message_details
'
