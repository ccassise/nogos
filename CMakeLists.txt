cmake_minimum_required(VERSION 3.0.0)
project(nogos VERSION 0.1.0 LANGUAGES C)

include(CTest)
enable_testing()

list(APPEND wflags
	-Wall
	-Wextra
	-Wshadow
	-Wdouble-promotion
	-Wconversion
	-Wpedantic
	-Wcast-align
	-Wstrict-prototypes

	-Wwrite-strings
	-Winit-self
	-Wformat=2
	-Wstrict-overflow=2
	-Wcast-qual
	-Wundef
	-Wswitch-default
	-fstack-protector-strong
	-Wparentheses
	-Wformat-overflow
	-Wunused-macros
	-Wswitch-enum
)

list(APPEND sanitizers
	-fsanitize=address
	-fsanitize=undefined
	-fno-sanitize-recover=all
	-fsanitize=float-divide-by-zero
	-fsanitize=float-cast-overflow
	-fno-sanitize=null
	-fno-sanitize=alignment
)

list(APPEND srcfiles
	${PROJECT_SOURCE_DIR}/src/board.c
	${PROJECT_SOURCE_DIR}/src/context.c
	${PROJECT_SOURCE_DIR}/src/lobby.c
	${PROJECT_SOURCE_DIR}/src/player.c
	${PROJECT_SOURCE_DIR}/src/protocol.c
	${PROJECT_SOURCE_DIR}/src/queue.c
)

add_subdirectory(src)
add_subdirectory(test)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
