# FindGPGME - Lookup native libgpgme installation.
#
# This module provides the following imported library targets:
#
# - GPGME::libgpgme
#
#   Target encapsulating the libgpgme usage requirements, if found.

include(FindPackageHandleStandardArgs)

find_package(PkgConfig QUIET)

if(PKG_CONFIG_FOUND)
	pkg_check_modules(PC_GPGME QUIET gpgme)
endif()

find_path(GPGME_INCLUDE_DIR NAMES gpgme.h HINTS ${PC_GPGME_INCLUDE_DIRS})
find_library(GPGME_LIBRARY NAMES libgpgme.so HINTS ${PC_GPGME_LIBRARY_DIRS})

set(GPGME_VERSION ${PC_GPGME_VERSION})

find_package_handle_standard_args(
	GPGME
	REQUIRED_VARS
		GPGME_LIBRARY
		GPGME_INCLUDE_DIR
	VERSION_VAR
		GPGME_VERSION
)

if(GPGME_FOUND AND NOT TARGET GPGME::libgpgme)
	add_library(GPGME::libgpgme UNKNOWN IMPORTED)
	set_target_properties(GPGME::libgpgme PROPERTIES
		IMPORTED_LOCATION "${GPGME_LIBRARY}"
		INTERFACE_COMPILE_OPTIONS "${PC_GPGME_CFLAGS_OTHER}"
		INTERFACE_INCLUDE_DIRECTORIES "${GPGME_INCLUDE_DIR}"
	)
endif()

mark_as_advanced(GPGME_INCLUDE_DIR GPGME_LIBRARY)
