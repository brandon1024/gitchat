#!/usr/bin/env bash

. ./test-lib.sh

assert_success 'git chat init -h should exit with status 0' '
	git-chat init -h
'

assert_success 'git chat init -h should display usage info' '
	reset_trash_dir
' '
	git-chat init -h | grep '\''^usage: git chat init'\''
'

assert_success 'git chat init with unknown flag should exit with status 1 and print useful message' '
	reset_trash_dir
' '
	git chat init --test 2>err
	test "$?" = "1" &&
	grep "error: unknown option '\''--test'\''" err
'

assert_success 'git chat init should initialize empty git repository' '
	reset_trash_dir
' '
	git chat init >out &&
	grep "Initialized empty Git repository" out &&
	grep "Successfully initialized git-chat space." out
'

assert_success 'reinitializing a git-chat space should not overwrite .git-chat or .keys directory' '
	reset_trash_dir
' '
	git chat init &&
	echo "my space" >>.git-chat/description &&
	echo "my keys" >.keys/key.gpg &&
	md5sum .git-chat/* >expected_git_chat &&
	md5sum .keys/* >expected_keys &&
	! git chat init &&
	md5sum .git-chat/* >actual_git_chat &&
	md5sum .keys/* >actual_keys &&
	cmp -s expected_git_chat actual_git_chat &&
	cmp -s expected_keys actual_keys
'

assert_success 'git chat init -q should not print to stdout' '
	reset_trash_dir
' '
	git chat init -q >out 2>err &&
	[ ! -s out ]
'

assert_success 'git chat init should create a single commit on master' '
	reset_trash_dir
' '
	git chat init &&
	git show --format="%s" -s >out &&
	grep "You have reached the beginning of time." out &&
	git rev-list --count HEAD >out
	[ "$(cat out)" -eq "1" ]
'

assert_success 'git chat init should create the expected directory structure' '
	reset_trash_dir
' '
	git chat init &&
	[ -d ".git" ] &&
	[ -d ".git/chat-cache" ] &&
	[ -d ".git-chat" ] &&
	[ -d ".keys" ] &&
	[ -f ".git-chat/config" ] &&
	[ -f ".git-chat/description" ]
'

assert_success 'git chat init -n with name should set the master channel name' '
	reset_trash_dir
' '
	git chat init -n "test-name" &&
	grep "name = test-name" .git-chat/config
'

assert_success 'git chat init --name with name should set the master channel name' '
	reset_trash_dir
' '
	git chat init --name "test-name-2" &&
	grep "name = test-name-2" .git-chat/config
'

assert_success 'git chat init -d with description should set the space description' '
	reset_trash_dir
' '
	git chat init -d "test-description" &&
	[ "$(cat .git-chat/description)" = "test-description" ]
'

assert_success 'git chat init --description with description should set the space description' '
	reset_trash_dir
' '
	git chat init --description "test-description-2" &&
	[ "$(cat .git-chat/description)" = "test-description-2" ]
'

assert_success 'git chat init should update config first with username' '
	reset_trash_dir &&
	git config --file ".gitconfig" user.username "test-username" &&
	git config --file ".gitconfig" user.email "test.email@test.com" &&
	git config --file ".gitconfig" user.name "test-name"
' '
	GIT_CONFIG=$(pwd)/.gitconfig GIT_CONFIG_NOSYSTEM=1 git chat init &&
	grep "createdby = test-username" .git-chat/config
'

assert_success 'git chat init should update config with email if no username found' '
	reset_trash_dir &&
	git config --file ".gitconfig" user.email "test.email@test.com" &&
	git config --file ".gitconfig" user.name "test-name"
' '
	GIT_CONFIG=$(pwd)/.gitconfig GIT_CONFIG_NOSYSTEM=1 git chat init &&
	grep "createdby = test.email@test.com" .git-chat/config
'

assert_success 'git chat init should update config with name if no username or email found' '
	reset_trash_dir &&
	git config --file ".gitconfig" user.name "test-name"
' '
	GIT_CONFIG=$(pwd)/.gitconfig GIT_CONFIG_NOSYSTEM=1 git chat init &&
	grep "createdby = test-name" .git-chat/config
'
