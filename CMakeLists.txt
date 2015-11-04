# Copyright (c) 2015 Fabian Schuiki
cmake_minimum_required(VERSION 2.8)
project(low)

if (CMAKE_BUILD_TYPE)
	message("## low ${CMAKE_BUILD_TYPE} build")
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
	src/grammar.c
	src/grammar_rules.c
	src/lexer.c
	src/main.c
	src/parser.c
	${CMAKE_BINARY_DIR}/parser_states.c
)

install(TARGETS lowc RUNTIME DESTINATION bin)
