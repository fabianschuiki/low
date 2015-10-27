# Copyright (c) 2015 Fabian Schuiki
cmake_minimum_required(VERSION 2.8)
project(low)

if (CMAKE_BUILD_TYPE)
	message("## low ${CMAKE_BUILD_TYPE} build")
endif()

add_definitions(-Wall)

add_executable(lowc
	src/main.c
	src/lexer.c
)

install(TARGETS lowc RUNTIME DESTINATION bin)
