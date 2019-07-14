# Integration Test Library
#
# All test library functions should exist here, and must be sourced by each
# integration test.

assert_zero_exit () {
    eval $1 > /dev/null
    if [ $? -ne 0 ]; then
        echo "'$1'" failed with exit status $?
        exit 1
    fi
}