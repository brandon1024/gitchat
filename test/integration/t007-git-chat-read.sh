#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git chat read -h should display usage info' '
	git chat read -h &&
	git chat read --help >out &&
	grep '\''^usage: git chat read'\'' out
'

assert_success 'git chat read must fail if not in git-chat space' '
	reset_trash_dir
' '
	! git chat read 2>err &&
	grep "Where are you? It doesn'\''t look like you'\''re in the right directory." err
'

assert_success 'git chat read in plain git repository must fail' '
	reset_trash_dir &&
	git init
' '
	! git chat read 2>err &&
	grep "Where are you? It doesn'\''t look like you'\''re in the right directory." err
'

assert_success 'git chat read must correctly display plaintext messages' '
	reset_trash_dir &&
	git chat init
' '
	git chat read >out &&
	grep "[*PLN*]" out &&
	git show -s --format="%B" >commit_msg &&
	grep -f commit_msg out
'

assert_success 'git chat read must successfully decrypt message privy to user' '
	setup_test_gpg &&
	git chat import-key -f "$TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg"
' '
	git chat message -m "hello world" &&
	PAGER=/usr/bin/cat git chat --passphrase password read >out &&
	grep "hello world" out
'

assert_success 'git chat read with message hash should print only that message' '
	setup_test_gpg
' '
	PAGER=/usr/bin/cat git chat --passphrase password read HEAD >out &&
	grep "hello world" out &&
	git chat read HEAD^ >out &&
	grep "joined the channel" out
'

assert_success 'git chat read should print ciphertext when cannot be decrypted' '
	reset_trash_dir &&
	setup_test_gpg &&
	git chat init &&
	git chat import-key -f "$TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg"
' '
	git chat message -m "hello world" &&
	setup_test_gpg "$TEST_RESOURCES_DIR/gpgkeys/ajones_noexpire.gpg" &&
	PAGER=/usr/bin/cat git chat --passphrase password read HEAD >out &&
	grep -v "hello world" out &&
	grep "message could not be decrypted" out
'

assert_success 'git chat read with message limit should print no more than that number of messages' '
	reset_trash_dir &&
	setup_test_gpg &&
	git chat init &&
	git chat import-key -f "$TEST_RESOURCES_DIR/gpgkeys/test_user.pub.gpg"
' '
	git chat message -m "hello world 1" &&
	git chat message -m "hello world 2" &&
	PAGER=/usr/bin/cat git chat --passphrase password read --max-count 1 >out &&
	grep "hello world 2" out &&
	grep -v "hello world 1" out
'

assert_success 'git chat read with message limit less than zero should print all messages' '
	setup_test_gpg
' '
	PAGER=/usr/bin/cat git chat --passphrase password read --max-count -1 >out &&
	grep "hello world 1" out &&
	grep "hello world 2" out
'

assert_success 'git chat read should print colored headers when output is a tty' '
	setup_test_gpg
' '
	GIT_CHAT_PAGER=/usr/bin/cat script -qec "git chat --passphrase password read --max-count 1" out &&
	grep -P "\033\[32m\[.*DEC.*\]\033\[0m" out
'

assert_success 'git chat read should print colored headers when output is a tty' '
	setup_test_gpg
' '
	GIT_CHAT_PAGER=/usr/bin/cat git chat --passphrase password read --max-count 1 >out &&
	grep -v -P "\033\[32m\[.*DEC.*\]\033\[0m" out
'
