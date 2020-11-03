#!/usr/bin/env bash

TOOL_OPTIONS="${TOOL_OPTIONS} --quiet"
TOOL_OPTIONS="${TOOL_OPTIONS} --tool=memcheck"
TOOL_OPTIONS="${TOOL_OPTIONS} --gen-suppressions=all"
TOOL_OPTIONS="${TOOL_OPTIONS} --leak-check=full"
TOOL_OPTIONS="${TOOL_OPTIONS} --show-leak-kinds=definite,possible"
TOOL_OPTIONS="${TOOL_OPTIONS} --track-origins=yes"
TOOL_OPTIONS="${TOOL_OPTIONS} --vgdb=no"
TOOL_OPTIONS="${TOOL_OPTIONS} --error-exitcode=1"

# If caller overrides valgrind options
if [ ! -z "${VALGRIND_TOOL_OPTIONS}" ]; then
	TOOL_OPTIONS="${VALGRIND_TOOL_OPTIONS}"
fi

exec valgrind ${TOOL_OPTIONS} ${VALGRIND_TARGET} "$@"
