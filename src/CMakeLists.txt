add_executable(
	nogos
	main.c
	${SRC_FILES}
)

set_property(TARGET nogos PROPERTY C_STANDARD ${C_STD})

target_compile_options(nogos PRIVATE ${WFLAGS} ${SANITIZERS})

target_link_options(nogos PRIVATE ${SANITIZERS} ${LIBSANITIZER})
