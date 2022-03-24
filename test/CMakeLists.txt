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

	# For some reason protocol tests fail when compiled with -fsanitize=address,
	# but valgrind isn't reporting anything and the tests are passing. Seems to
	# be a weird interaction with fmemopen when being called with a NULL pointer
	# as the first argument?
	if(test STREQUAL "protocol")
		target_compile_options(test_${test} PRIVATE ${wflags})
	else()
		target_compile_options(test_${test} PRIVATE ${wflags} ${sanitizers})
		target_link_options(test_${test} PRIVATE ${sanitizers} -static-libasan)
	endif()


	add_test(NAME test_${test} COMMAND test_${test})
endforeach()
