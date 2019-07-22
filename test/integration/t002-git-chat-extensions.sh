#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git-chat should look for extensions when invoked with unrecognized subcommand' '
	cat >git-chat-extension <<-\EOF &&
	#!/bin/sh
	echo hello world
	exit 0
	EOF
	chmod +x git-chat-extension
' '
	PATH=$PATH:$(pwd) git chat extension | grep "hello world"
'

assert_success 'git-chat should exit with same exit status as extension if extension exits with status 0' '
	cat >git-chat-extension <<-\EOF &&
	#!/bin/sh
	echo hello world
	exit 0
	EOF
	chmod +x git-chat-extension
' '
	PATH=$PATH:$(pwd) git chat extension
	test "$?" = "0"
'

assert_success 'git-chat should exit with same exit status as extension if extension exits with status 1' '
	cat >git-chat-extension <<-\EOF &&
	#!/bin/sh
	echo hello world
	exit 27
	EOF
	chmod +x git-chat-extension
' '
	PATH=$PATH:$(pwd) git chat extension
	test "$?" = "27"
'

assert_success 'git-chat should not run extension on path if not executable' '
	cat >git-chat-noex <<-\EOF &&
	#!/bin/sh
	echo hello world
	exit 0
	EOF
	chmod -x git-chat-noex
' '
	PATH=$PATH:$(pwd) git chat noex
	test "$?" != "0"
'

assert_success 'git-chat should fail if no extension could be found on the path' '
	git chat unknown-extension 2>&1 | grep '\''^error: '\''
'
