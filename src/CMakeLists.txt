add_executable(
	nogos
	main.c
	${srcfiles}
)

set_property(TARGET nogos PROPERTY C_STANDARD 99)

# target_compile_options(nogos PRIVATE ${wflags})

target_compile_options(nogos PRIVATE ${wflags} ${sanitizers})
target_link_options(nogos PRIVATE ${sanitizers} -static-libasan)
