#!/usr/bin/env bash

. ./test-lib.sh

assert_success 'git chat init -h should exit with status 0' '
	git-chat init -h
'

assert_success 'git chat init -h should exit with status 0' '
	git-chat init -h | grep '\''^usage: git chat init'\''
'

assert_success 'git chat init should initialize empty git directory' '
	reset_trash_dir
' '
	git chat init >out &&
	grep "Initialized empty Git repository" out &&
	grep "Successfully initialized git-chat space." out
'
