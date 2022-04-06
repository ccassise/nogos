list(APPEND tests
	context
	lobby
	queue
)

foreach(test IN LISTS tests)
	add_executable(test_${test} test_${test}.c ${SRC_FILES})

	set_property(TARGET test_${test} PROPERTY C_STANDARD ${C_STD})

	target_compile_options(test_${test} PRIVATE ${WFLAGS} ${SANITIZERS})
	target_include_directories(test_${test} PRIVATE ${PROJECT_SOURCE_DIR}/src)
	target_link_libraries(test_${test} PRIVATE libnogo)
	target_link_options(test_${test} PRIVATE ${SANITIZERS} ${SANITIZER_LIB})

	add_test(NAME test_${test} COMMAND test_${test})
endforeach()
