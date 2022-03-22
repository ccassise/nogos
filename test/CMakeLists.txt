list(APPEND tests
	board
	lobby
	message_queue
	protocol
)

foreach(test IN LISTS tests)
	add_executable(test_${test} test_${test}.c ${srcfiles})

	set_property(TARGET test_${test} PROPERTY C_STANDARD 99)

	target_include_directories(test_${test} PRIVATE ${PROJECT_SOURCE_DIR}/src)
	target_compile_options(test_${test} PRIVATE ${wflags} ${sanitizers})
	target_link_options(test_${test} PRIVATE ${sanitizers} -static-libasan)

	add_test(NAME test_${test} COMMAND test_${test})
endforeach()
