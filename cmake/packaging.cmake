set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A Git-Based Command-Line Messaging Application")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_PACKAGE_CONTACT "Brandon Richardson <brandon1024.br@gmail.com>")

if (NOT UNIX)
	message(WARNING "cannot generate package: unrecognized system")
	return()
endif ()

# Determine distribution and release
execute_process(COMMAND lsb_release -si
		OUTPUT_VARIABLE distribution
		RESULT_VARIABLE lsb_release_err
		OUTPUT_STRIP_TRAILING_WHITESPACE)

if (NOT lsb_release_err EQUAL 0)
	message(WARNING "cannot determine distribution; lsb_release missing?")
	return()
endif ()

if (distribution STREQUAL "Debian" OR distribution STREQUAL "Ubuntu")
	include(cmake/package-debian.cmake)
elseif(distribution STREQUAL "CentOS")
	include(cmake/package-centos.cmake)
else()
	message(WARNING "unrecognized system ${distribution}; generating TGZ")
	set(CPACK_GENERATOR "TGZ")
endif ()
