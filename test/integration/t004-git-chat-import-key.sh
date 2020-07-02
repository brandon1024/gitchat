#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git chat import-key -h should display usage info' '
	reset_trash_dir &&
	git chat init
' '
	git chat import-key -h &&
	git chat import-key --help >out &&
	grep '\''^usage: git chat import-key'\'' out
'

assert_success 'git chat import-key should import all keys with supplied fingerprints' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg
' '
	git chat import-key 49A9FED4003D28CB5D8CF96748F6785D011F494B 033EACE8B8197D39D269851DE3B323373AC40A46 41C4155B702593BFF6D7D140AB4F26E5A765DFDC &&
	GNUPGHOME="$(pwd)/.git/.gnupg" gpg --list-keys 49A9FED4003D28CB5D8CF96748F6785D011F494B &&
	GNUPGHOME="$(pwd)/.git/.gnupg" gpg --list-keys 033EACE8B8197D39D269851DE3B323373AC40A46 &&
	GNUPGHOME="$(pwd)/.git/.gnupg" gpg --list-keys 41C4155B702593BFF6D7D140AB4F26E5A765DFDC
'

assert_success 'git-chat import-key should correctly import key from file' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg
' '
	git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/ajones_noexpire.pub.gpg &&
	GNUPGHOME="$(pwd)/.git/.gnupg" gpg --list-keys 49A9FED4003D28CB5D8CF96748F6785D011F494B &&
	git chat import-key --file $TEST_RESOURCES_DIR/gpgkeys/jdoe_noexpire.pub.gpg -f $TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg &&
	GNUPGHOME="$(pwd)/.git/.gnupg" gpg --list-keys 033EACE8B8197D39D269851DE3B323373AC40A46 &&
	GNUPGHOME="$(pwd)/.git/.gnupg" gpg --list-keys 41C4155B702593BFF6D7D140AB4F26E5A765DFDC
'

assert_success 'git-chat import-key should fail when given file paths and fingerprints' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg
' '
	! git chat import-key -f $TEST_RESOURCES_DIR/gpgkeys/ajones_noexpire.pub.gpg 41C4155B702593BFF6D7D140AB4F26E5A765DFDC 2>err &&
	grep "mutually exclusive operations" err &&
	git chat import-key 41C4155B702593BFF6D7D140AB4F26E5A765DFDC --file $TEST_RESOURCES_DIR/gpgkeys/ajones_noexpire.pub.gpg 2>err >out &&
	grep "could not find public gpg key with fingerprint" err &&
	grep "41C4155B702593BFF6D7D140AB4F26E5A765DFDC" out &&
	GNUPGHOME="$(pwd)/.git/.gnupg" gpg --list-keys 41C4155B702593BFF6D7D140AB4F26E5A765DFDC
'

assert_success 'git-chat import-key should fail if key is already imported' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg &&
	gpg --import $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg
' '
	git chat import-key 41C4155B702593BFF6D7D140AB4F26E5A765DFDC &&
	GNUPGHOME="$(pwd)/.git/.gnupg" gpg --list-keys 41C4155B702593BFF6D7D140AB4F26E5A765DFDC &&
	! git chat import-key 41C4155B702593BFF6D7D140AB4F26E5A765DFDC 2> err &&
	grep "may have already been imported" err
'

assert_success 'git-chat import-key should fail if key file has been corrupted' '
	reset_trash_dir &&
	git chat init &&
	setup_test_gpg
' '
	cp $TEST_RESOURCES_DIR/gpgkeys/ajones_noexpire.pub.gpg corrupted.pub.gpg &&
	sed -i'\''.orig'\'' -e '\''5d'\'' corrupted.pub.gpg &&
	! git chat import-key -f corrupted.pub.gpg 2>err &&
	grep "unable to import key from file" err
'
