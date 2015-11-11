# Copyright (c) 2015 Fabian Schuiki
cmake_minimum_required(VERSION 2.8)
project(low)

if (CMAKE_BUILD_TYPE)
	message("## low ${CMAKE_BUILD_TYPE} build")
endif()


set(LLVM_HINT_PATHS
	/
	/usr
	/usr/local
	/opt
	/usr/local/opt/llvm # Homebrew
)
set(LLVM_LIBS core bitwriter target analysis)

if (NOT LLVM_CONFIG_BIN)
	find_program(LLVM_CONFIG_BIN llvm-config HINTS ${LLVM_HINT_PATHS} ENV LLVM_DIR PATH_SUFFIXES bin)
endif()

if (NOT LLVM_CONFIG_BIN)
	message(FATAL_ERROR "Unable to find LLVM")
endif()
message(STATUS "Found LLVM (${LLVM_CONFIG_BIN})")

execute_process(COMMAND ${LLVM_CONFIG_BIN} --cppflags OUTPUT_VARIABLE TEMP)
string(REGEX REPLACE "[ \t\n\r]+" " " LLVM_COMPILE_FLAGS ${TEMP})

execute_process(COMMAND ${LLVM_CONFIG_BIN} --ldflags --libs ${LLVM_LIBS} --system-libs OUTPUT_VARIABLE TEMP)
string(REGEX REPLACE "[ \t\n\r]+" " " LLVM_LINK_FLAGS ${TEMP})

if (NOT APPLE)
	set(LLVM_LINK_FLAGS "${LLVM_LINK_FLAGS} -lLLVM")
endif()


add_definitions(-Wall)
include_directories(src)

add_executable(parser-generator
	src/array.c
	src/grammar.c
	src/parser_generator.c
)

add_custom_command(
	DEPENDS parser-generator
	OUTPUT ${CMAKE_BINARY_DIR}/parser_states.c
	COMMAND ${CMAKE_BINARY_DIR}/parser-generator > ${CMAKE_BINARY_DIR}/parser_states.c
	COMMENT "Generating parser states"
)

add_executable(lowc
	src/array.c
	src/ast.c
	src/codegen.c
	src/grammar.c
	src/grammar_rules.c
	src/lexer.c
	src/main.c
	src/parser.c
	${CMAKE_BINARY_DIR}/parser_states.c
)

set_target_properties(lowc PROPERTIES
	LINKER_LANGUAGE "CXX"
	COMPILE_FLAGS ${LLVM_COMPILE_FLAGS}
	LINK_FLAGS ${LLVM_LINK_FLAGS}
)

install(TARGETS lowc RUNTIME DESTINATION bin)
