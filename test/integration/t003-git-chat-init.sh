#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git chat init -h should display usage info' '
	reset_trash_dir
' '
	git-chat init -h &&
	git-chat init --help >out &&
	grep '\''^usage: git chat init'\'' out
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

assert_success 'reinitializing a git-chat space should not overwrite .git-chat' '
	reset_trash_dir
' '
	git chat init &&
	cp $TEST_RESOURCES_DIR/gpgkeys/*.pub.gpg .git-chat/keys &&
	echo "my space" >>.git-chat/description &&
	shasum .git-chat/description >expected_git_chat_description &&
	shasum .git-chat/keys/* >expected_git_chat_keys &&
	! git chat init &&
	shasum .git-chat/description >actual_git_chat_description &&
	shasum .git-chat/keys/* >actual_git_chat_keys &&
	cmp -s expected_git_chat_description actual_git_chat_description &&
	cmp -s expected_git_chat_keys actual_git_chat_keys
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
	[ -d ".git-chat/keys" ] &&
	[ -f ".git-chat/config" ] &&
	[ -f ".git-chat/description" ]
'

assert_success 'git chat init -n with name should set the master channel name' '
	reset_trash_dir
' '
	git chat init -n "test-name" &&
	grep "name = \"test-name\"" .git-chat/config
'

assert_success 'git chat init --name with name should set the master channel name' '
	reset_trash_dir
' '
	git chat init --name "test-name-2" &&
	grep "name = \"test-name-2\"" .git-chat/config
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
	touch .gitconfig &&
	GIT_CONFIG=$(pwd)/.gitconfig
	git config --file "${GIT_CONFIG}" user.username "test-username" &&
	git config --file "${GIT_CONFIG}" user.email "test.email@test.com" &&
	git config --file "${GIT_CONFIG}" user.name "test-name"
' '
	git chat init &&
	grep "createdby = \"test-username\"" .git-chat/config
'

assert_success 'git chat init should update config with email if no username found' '
	reset_trash_dir &&
	touch .gitconfig &&
	GIT_CONFIG=$(pwd)/.gitconfig
	git config --file "${GIT_CONFIG}" user.email "test.email@test.com" &&
	git config --file "${GIT_CONFIG}" user.name "test-name"
' '
	git chat init &&
	grep "createdby = \"test.email@test.com\"" .git-chat/config
'

assert_success 'git chat init should update config with name if no username or email found' '
	reset_trash_dir &&
	touch .gitconfig &&
	GIT_CONFIG=$(pwd)/.gitconfig
	git config --file "${GIT_CONFIG}" user.name "test-name"
' '
	git chat init &&
	grep "createdby = \"test-name\"" .git-chat/config
'
