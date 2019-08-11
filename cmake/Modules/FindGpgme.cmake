# This script was taken and adapted from rpm-software-management/librepo:
# https://github.com/rpm-software-management/librepo/blob/master/cmake/Modules/FindGpgme.cmake
#
# Try to find the gpgme library:
#	There's also three variants: gpgme{,-pthread,-pth}. The variant used determines the multithreaded use possible:
#		- gpgme:         no multithreading support available
#		- gpgme-pthread: multithreading available using POSIX threads
#		- gpgme-pth:     multithreading available using GNU PTH (cooperative multithreading)
#	- GPGME_{VANILLA,PTH,PTHREAD}_{FOUND,LIBRARIES} will be set for each of the above
#	- GPGME_INCLUDES is the same for all of the above
#	- GPGME_FOUND is set if any of the above was found
#
#	GPGME_LIBRARY_DIR - the directory where the libraries are located

SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS_gpgme_saved ${CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS})
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

MACRO(macro_bool_to_bool FOUND_VAR)
	FOREACH (_current_VAR ${ARGN})
		IF (${FOUND_VAR})
			SET(${_current_VAR} TRUE)
		ELSE ()
			SET(${_current_VAR} FALSE)
		ENDIF ()
	ENDFOREACH ()
ENDMACRO()

SET(_seem_to_have_cached_gpgme false)
IF (GPGME_INCLUDES)
	IF (GPGME_VANILLA_LIBRARIES OR GPGME_PTHREAD_LIBRARIES OR GPGME_PTH_LIBRARIES)
		SET(_seem_to_have_cached_gpgme true)
	ENDIF ()
ENDIF ()

IF (_seem_to_have_cached_gpgme)
	macro_bool_to_bool(GPGME_VANILLA_LIBRARIES GPGME_VANILLA_FOUND)
	macro_bool_to_bool(GPGME_PTHREAD_LIBRARIES GPGME_PTHREAD_FOUND)
	macro_bool_to_bool(GPGME_PTH_LIBRARIES GPGME_PTH_FOUND)

	IF (GPGME_VANILLA_FOUND OR GPGME_PTHREAD_FOUND OR GPGME_PTH_FOUND)
		SET(GPGME_FOUND true)
	ELSE ()
		SET(GPGME_FOUND false)
	ENDIF ()
ELSE ()
	SET(GPGME_FOUND false)
	SET(GPGME_VANILLA_FOUND false)
	SET(GPGME_PTHREAD_FOUND false)
	SET(GPGME_PTH_FOUND false)

	FIND_PROGRAM(_GPGMECONFIG_EXECUTABLE NAMES gpgme-config)
	IF (_GPGMECONFIG_EXECUTABLE)
		MESSAGE(STATUS "Found gpgme-config at ${_GPGMECONFIG_EXECUTABLE}")
		EXEC_PROGRAM(${_GPGMECONFIG_EXECUTABLE} ARGS --version OUTPUT_VARIABLE GPGME_VERSION)

		MESSAGE(STATUS "Found gpgme v${GPGME_VERSION}, checking for flavours...")
		EXEC_PROGRAM(${_GPGMECONFIG_EXECUTABLE} ARGS --libs OUTPUT_VARIABLE _gpgme_config_vanilla_libs RETURN_VALUE _ret)
		IF (_ret)
			SET(_gpgme_config_vanilla_libs)
		ENDIF ()

		EXEC_PROGRAM(${_GPGMECONFIG_EXECUTABLE} ARGS --thread=pthread --libs OUTPUT_VARIABLE _gpgme_config_pthread_libs RETURN_VALUE _ret)
		IF (_ret)
			SET(_gpgme_config_pthread_libs)
		ENDIF ()

		EXEC_PROGRAM(${_GPGMECONFIG_EXECUTABLE} ARGS --thread=pth --libs OUTPUT_VARIABLE _gpgme_config_pth_libs RETURN_VALUE _ret)
		IF (_ret)
			SET(_gpgme_config_pth_libs)
		ENDIF ()

		# append -lgpg-error to the list of libraries, if necessary
		FOREACH (_flavour vanilla pthread pth)
			IF (_gpgme_config_${_flavour}_libs AND NOT _gpgme_config_${_flavour}_libs MATCHES "lgpg-error")
				SET(_gpgme_config_${_flavour}_libs "${_gpgme_config_${_flavour}_libs} -lgpg-error")
			ENDIF ()
		ENDFOREACH ()

		IF (_gpgme_config_vanilla_libs OR _gpgme_config_pthread_libs OR _gpgme_config_pth_libs)
			EXEC_PROGRAM(${_GPGMECONFIG_EXECUTABLE} ARGS --cflags OUTPUT_VARIABLE _GPGME_CFLAGS)

			IF (_GPGME_CFLAGS)
				STRING(REGEX REPLACE "(\r?\n)+$" " " _GPGME_CFLAGS "${_GPGME_CFLAGS}")
				STRING(REGEX REPLACE " *-I" ";" GPGME_INCLUDES "${_GPGME_CFLAGS}")
			ENDIF ()

			FOREACH (_flavour vanilla pthread pth)
				IF (_gpgme_config_${_flavour}_libs)
					SET(_gpgme_library_dirs)
					SET(_gpgme_library_names)
					STRING(TOUPPER "${_flavour}" _FLAVOUR)

					STRING(REGEX REPLACE " +" ";" _gpgme_config_${_flavour}_libs "${_gpgme_config_${_flavour}_libs}")
					FOREACH (_flag ${_gpgme_config_${_flavour}_libs})
						IF ("${_flag}" MATCHES "^-L")
							STRING(REGEX REPLACE "^-L" "" _dir "${_flag}")
							file(TO_CMAKE_PATH "${_dir}" _dir)
							SET(_gpgme_library_dirs ${_gpgme_library_dirs} "${_dir}")
						ELSEIF ("${_flag}" MATCHES "^-l")
							STRING(REGEX REPLACE "^-l" "" _name "${_flag}")
							SET(_gpgme_library_names ${_gpgme_library_names} "${_name}")
						ENDIF ()
					ENDFOREACH ()

					SET(GPGME_${_FLAVOUR}_FOUND true)
					FOREACH (_name ${_gpgme_library_names})
						SET(_gpgme_${_name}_lib)

						# if -L options were given, look only there
						IF (_gpgme_library_dirs)
							FIND_LIBRARY(_gpgme_${_name}_lib NAMES ${_name} PATHS ${_gpgme_library_dirs} NO_DEFAULT_PATH)
						ENDIF ()

						# if not found there, look in system directories
						IF (NOT _gpgme_${_name}_lib)
							FIND_LIBRARY(_gpgme_${_name}_lib NAMES ${_name})
						ENDIF ()

						# if still not found, then the whole flavour isn't found
						IF (NOT _gpgme_${_name}_lib)
							IF (GPGME_${_FLAVOUR}_FOUND)
								SET(GPGME_${_FLAVOUR}_FOUND false)
								SET(_not_found_reason "dependent library ${_name} wasn't found")
							ENDIF ()
						ENDIF ()

						SET(GPGME_${_FLAVOUR}_LIBRARIES ${GPGME_${_FLAVOUR}_LIBRARIES} "${_gpgme_${_name}_lib}")
					ENDFOREACH ()

					IF (GPGME_${_FLAVOUR}_FOUND)
						MESSAGE(STATUS " Found flavour '${_flavour}', checking whether it's usable...yes")
					ELSE ()
						MESSAGE(STATUS " Found flavour '${_flavour}', checking whether it's usable...no")
						MESSAGE(STATUS "  (${_not_found_reason})")
					ENDIF ()
				ENDIF ()
			ENDFOREACH (_flavour)

			SET(GPGME_INCLUDES ${GPGME_INCLUDES})
			SET(GPGME_VANILLA_LIBRARIES ${GPGME_VANILLA_LIBRARIES})
			SET(GPGME_PTHREAD_LIBRARIES ${GPGME_PTHREAD_LIBRARIES})
			SET(GPGME_PTH_LIBRARIES ${GPGME_PTH_LIBRARIES})

			IF (GPGME_VANILLA_FOUND OR GPGME_PTHREAD_FOUND OR GPGME_PTH_FOUND)
				SET(GPGME_FOUND true)
			ELSE ()
				SET(GPGME_FOUND false)
			ENDIF ()
		ENDIF ()
	ELSE ()
		MESSAGE(STATUS "Unable to find gpgme-config")
	ENDIF ()
ENDIF ()

SET(_gpgme_flavours "")
IF (GPGME_VANILLA_FOUND)
	SET(_gpgme_flavours "${_gpgme_flavours} vanilla")
ENDIF ()

IF (GPGME_GLIB_FOUND)
	SET(_gpgme_flavours "${_gpgme_flavours} Glib")
ENDIF ()

IF (GPGME_QT_FOUND)
	SET(_gpgme_flavours "${_gpgme_flavours} Qt")
ENDIF ()

IF (GPGME_PTHREAD_FOUND)
	SET(_gpgme_flavours "${_gpgme_flavours} pthread")
ENDIF ()

IF (GPGME_PTH_FOUND)
	SET(_gpgme_flavours "${_gpgme_flavours} pth")
ENDIF ()

# determine the library in one of the found flavours, can be reused e.g. by FindQgpgme.cmake, Alex
FOREACH (_currentFlavour vanilla glib qt pth pthread)
	IF (NOT GPGME_LIBRARY_DIR)
		get_filename_component(GPGME_LIBRARY_DIR "${_gpgme_${_currentFlavour}_lib}" PATH)
	ENDIF ()
ENDFOREACH ()

IF (NOT Gpgme_FIND_QUIETLY)
	IF (GPGME_FOUND)
		MESSAGE(STATUS "Usable gpgme flavours found: ${_gpgme_flavours}")
	ELSE ()
		MESSAGE(STATUS "No usable gpgme flavours found.")
	ENDIF ()

	macro_bool_to_bool(Gpgme_FIND_REQUIRED _req)
	SET(_gpgme_homepage "http://www.gnupg.org/related_software/gpgme")
ELSE ()
	IF (Gpgme_FIND_REQUIRED AND NOT GPGME_FOUND)
		MESSAGE(FATAL_ERROR "Did not find GPGME")
	ENDIF ()
ENDIF ()

SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS_gpgme_saved)
