list(APPEND tests
	board
	context
	lobby
	protocol
	queue
)

foreach(test IN LISTS tests)
	add_executable(test_${test} test_${test}.c ${SRC_FILES})

	set_property(TARGET test_${test} PROPERTY C_STANDARD ${C_STD})

	target_include_directories(test_${test} PRIVATE ${PROJECT_SOURCE_DIR}/src)

	target_compile_options(test_${test} PRIVATE ${WFLAGS} ${SANITIZERS})

	target_link_options(test_${test} PRIVATE ${SANITIZERS} ${LIBSANITIZER})

	add_test(NAME test_${test} COMMAND test_${test})
endforeach()
