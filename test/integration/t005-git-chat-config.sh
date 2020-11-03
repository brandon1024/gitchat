#!/usr/bin/env bash

source ./test-lib.sh

assert_success 'git chat config -h should show usage info' '
	reset_trash_dir
' '
	git chat config -h >out &&
	grep '\''^usage: git chat config'\'' out
'

assert_success 'git chat config uninitialized space should fail with appropriate message' '
	reset_trash_dir
' '
	! git chat config --get key 2>err &&
	! git chat config --get key &&
	grep "Where are you" err
'

assert_success 'git chat config --get with unknown config should print empty string' '
	reset_trash_dir &&
	git chat init
' '
	! git chat config --get unknown >out &&
	[ ! -s out ]
'

assert_success 'git chat config --get with known config should print config' '
	reset_trash_dir &&
	git chat init &&
	cat >.git-chat/config <<-\EOF
	[ mysimpleconfig ]
		simple = value
	EOF
' '
	git chat config --get mysimpleconfig.simple >out &&
	grep "value" out
'

assert_success 'git chat config --get should find all config entries' '
	reset_trash_dir &&
	git chat init &&
	cat >.git-chat/config <<-\EOF
	[ config1 ]
		simple1 = value1
		simple2 = value2
	[ config2 ]
		simple3 = value3
		simple4 = value4
	EOF
' '
	git chat config --get config1.simple1 &&
	git chat config --get config1.simple2 &&
	git chat config --get config2.simple3 &&
	git chat config --get config2.simple4
'

assert_success 'git chat config --set with existing key should set the config for that key' '
	reset_trash_dir &&
	git chat init &&
	cat >.git-chat/config <<-\EOF
	[ config1 ]
		simple1 = value
		simple2 = value1
	[ config2 ]
		simple3 = value
		simple4 = value1
	EOF
' '
	git chat config --set config1.simple2 "new value" &&
	grep "simple2 = \"new value\"" .git-chat/config &&
	! grep "simple2 = value" .git-chat/config &&
	git chat config --get config1.simple2 >out &&
	grep "new value" out
'

assert_success 'git chat config --set with unknown key should create new config' '
	reset_trash_dir &&
	git chat init
' '
	git chat config --set my.config "my config value" &&
	git chat config --get my.config >out &&
	cat out &&
	grep "my config value" out
'

assert_success 'git chat config --set with invalid key should fail with message' '
	reset_trash_dir &&
	git chat init
' '
	! git chat config --set '\''my.c$onfig.value'\'' "my config value" 2>err &&
	cat err &&
	grep "invalid key" err &&
	! git chat config --set "my.c onfig.value" "my config value" 2>err &&
	grep "invalid key" err &&
	! git chat config --set my.c$onfig. "my config value" 2>err &&
	grep "invalid key" err &&
	! git chat config --set my..value "my config value" 2>err &&
	grep "invalid key" err &&
	! git chat config --set .my.value "my config value" 2>err &&
	grep "invalid key" err
'

assert_success 'git chat config --unset with unknown key should exit with nonzero status' '
	! git chat config --unset unknown.config
'

assert_success 'git chat config --unset should remove the property from the file' '
	reset_trash_dir &&
	git chat init &&
	cat >.git-chat/config <<-\EOF
	[ config1 ]
		simple1 = value1
		simple2 = value2
	[ config2 ]
		simple3 = value3
		simple4 = value4
	EOF
' '
	git chat config --unset config1.simple1 &&
	! grep "simple1" .git-chat/config &&
	! grep "value1" .git-chat/config
'

assert_success 'git chat config --unset should remove section if no entries exist' '
	reset_trash_dir &&
	git chat init &&
	cat >.git-chat/config <<-\EOF
	[ config1 ]
		simple1 = value1
		simple2 = value2
	[ config2 ]
		simple3 = value3
		simple4 = value4
	EOF
' '
	git chat config --unset config1.simple1 &&
	git chat config --unset config1.simple2 &&
	! grep "config1" .git-chat/config
'

assert_success 'git chat config with invalid combination of options should fail' '
	reset_trash_dir &&
	git chat init &&
	cat >.git-chat/config <<-\EOF
	[ config1 ]
		simple1 = value1
		simple2 = value2
	[ config2 ]
		simple3 = value3
		simple4 = value4
	EOF
' '
	! git chat config --get --set config2.simple3 value &&
	! git chat config --get --unset config1.simple2 &&
	! git chat config --set config2.simple3 value --get-or-default config2.simple4 &&
	! git chat config --get --get-or-default config2.simple3 &&
	! git chat config --is-valid-config --is-valid-key channel.master.name &&
	! git chat config --unset config2.simple3 --set config2.simple3 value &&
	! git chat config --is-valid-config --set config2.simple3 value
'

assert_success 'git chat config --get-or-default should return default value' '
	reset_trash_dir &&
	git chat init
' '
	git chat config --get-or-default channel.master.name >out &&
	grep "master" out
'

assert_success 'git chat config --is-valid-key should exit with zero status if key is valid' '
	git chat config --is-valid-key channel.master.name &&
	git chat config --is-valid-key channel.dev.name &&
	git chat config --is-valid-key channel.random.name &&
	! git chat config --is-valid-key channel.name &&
	! git chat config --is-valid-key unknown &&
	! git chat config --is-valid-key '\''channel.ma&ster.name'\'' &&
	! git chat config --is-valid-key channel..name
'

assert_success 'git chat config --is-valid-config with invalid config should exit with nonzero status' '
	reset_trash_dir &&
	git chat init
' '
	cp $TEST_RESOURCES_DIR/bad_key_1.config .git-chat/config &&
	! git chat config --is-valid-config &&
	cp $TEST_RESOURCES_DIR/bad_key_2.config .git-chat/config &&
	! git chat config --is-valid-config &&
	cp $TEST_RESOURCES_DIR/bad_key_3.config .git-chat/config &&
	! git chat config --is-valid-config &&
	cp $TEST_RESOURCES_DIR/bad_key_4.config .git-chat/config &&
	! git chat config --is-valid-config &&
	cp $TEST_RESOURCES_DIR/bad_section_1.config .git-chat/config &&
	! git chat config --is-valid-config &&
	cp $TEST_RESOURCES_DIR/bad_section_2.config .git-chat/config &&
	! git chat config --is-valid-config &&
	cp $TEST_RESOURCES_DIR/bad_section_3.config .git-chat/config &&
	! git chat config --is-valid-config &&
	cp $TEST_RESOURCES_DIR/good_1.config .git-chat/config &&
	! git chat config --is-valid-config &&
	cp $TEST_RESOURCES_DIR/good_2.config .git-chat/config &&
	! git chat config --is-valid-config
'

assert_success 'git chat config --is-valid-config with valid config should exit with zero status' '
	reset_trash_dir &&
	git chat init
' '
	cp $TEST_RESOURCES_DIR/good_3.config .git-chat/config &&
	git chat config --is-valid-config
'
