# Copyright (c) 2015 Fabian Schuiki
cmake_minimum_required(VERSION 2.8)
project(low)

if (CMAKE_BUILD_TYPE)
	message("## low ${CMAKE_BUILD_TYPE} build")
endif()

add_definitions(-Wall)

add_executable(parser-generator
	src/array.c
	src/ast.c
	src/grammar.c
	src/parser_generator.c
)

add_executable(lowc
	src/array.c
	src/ast.c
	src/grammar.c
	src/lexer.c
	src/main.c
	src/parser.c
)

install(TARGETS lowc RUNTIME DESTINATION bin)
