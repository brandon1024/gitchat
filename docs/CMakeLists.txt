#
# Configure Man Page Installation
#
find_program(GZIP_EXECUTABLE gzip)
add_custom_target(docs ALL)

#
# Sections and Template Strings
#
string(TIMESTAMP CMAKE_COMPILATION_DATE "%d/%m/%Y")
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sections/directory-structure.section DOCS_FILE_DIRECTORY_LAYOUT_SECTION)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sections/reporting-bugs.section DOCS_REPORTING_BUGS_SECTION)
file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sections/authors.section DOCS_AUTHORS_SECTION)

#
# Configure Build and Install Target
#
foreach(SECTION 1 2 3 4 5 6 7 8)
	file(GLOB MAN_PAGES ${CMAKE_CURRENT_SOURCE_DIR}/*.${SECTION})

	foreach(FILE ${MAN_PAGES})
		get_filename_component(FILENAME ${FILE} NAME)
		configure_file(${FILE} ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME})

		install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FILENAME} DESTINATION share/man/man${SECTION})
	endforeach()
endforeach()
