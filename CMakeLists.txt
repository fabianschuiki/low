# Copyright (c) 2015 Fabian Schuiki, Thomas Richner
cmake_minimum_required(VERSION 2.8)
project(low)

if (CMAKE_BUILD_TYPE)
	message("## low ${CMAKE_BUILD_TYPE} build")
endif()



# Find the LLVM configuration tool and libraries.
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
	message(FATAL_ERROR "Unable to find llvm-config")
endif()
message(STATUS "Found llvm-config (${LLVM_CONFIG_BIN})")

macro(llvm_config OUTVAR)
	execute_process(COMMAND ${LLVM_CONFIG_BIN} ${ARGN} OUTPUT_VARIABLE TEMP OUTPUT_STRIP_TRAILING_WHITESPACE)
	if (NOT TEMP)
		message(FATAL_ERROR "Execution of llvm-config failed")
	endif()
	string(REGEX REPLACE "[ \t\n\r]+" " " ${OUTVAR} ${TEMP})
endmacro()

llvm_config(LLVM_COMPILE_FLAGS --cppflags)
llvm_config(LLVM_LINK_FLAGS --ldflags)
llvm_config(LLVM_SYSTEM_LIBRARIES --system-libs)

# Linking against LLVM is somewhat tricky, since different platforms use
# different libraries and setups. On most Linux systems, a shared libLLVM is
# provided that incorporates all of the modules. Unfortunately, the naming of
# that library is not particularly uniform (libLLVM-3.6.so on Ubuntu, libLLVM.so
# on Arch Linux). Mac OS X on the other hand has no such shared library and
# requires direct linking against the components.
if (APPLE)
	llvm_config(LLVM_LIBRARIES --libs ${LLVM_LIBS})
	message(STATUS "Manually linking against LLVM libraries")
else()
	llvm_config(LLVM_LIBDIR --libdir)
	find_library(LLVM_LIB NAMES LLVM LLVM-3.6 LLVM-3.5 HINTS ${LLVM_LIBDIR} ${LLVM_HINT_PATHS} ENV LLVM_DIR PATH_SUFFIXES lib)
	if (NOT LLVM_LIB)
		message(FATAL_ERROR "Unable to find libLLVM")
	endif()
	message(STATUS "Linking against shared LLVM library (${LLVM_LIB})")
	set(LLVM_LIBRARIES ${LLVM_LIB})
endif()



# Be pretty pedantic about issues in the code. We want a clean build with no
# warnings.
add_definitions(-Wall -Werror=implicit-function-declaration)
include_directories(src)



# Build the state generator for the parser automaton and add that as a separate
# build step the lowc depends on.
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



# Build the Low compiler.
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

target_link_libraries(lowc ${LLVM_LIBRARIES} ${LLVM_SYSTEM_LIBRARIES})

set_target_properties(lowc PROPERTIES
	LINKER_LANGUAGE "CXX"
	COMPILE_FLAGS ${LLVM_COMPILE_FLAGS}
	LINK_FLAGS ${LLVM_LINK_FLAGS}
)



# Testing
#cmake_policy(SET CMP0037 OLD)
add_custom_target(test
	LOWC=${CMAKE_CURRENT_BINARY_DIR}/lowc ${CMAKE_CURRENT_SOURCE_DIR}/test/run_tests.sh
	DEPENDS lowc
	COMMENT "Running tests"
)



# Installation
install(TARGETS lowc RUNTIME DESTINATION bin)
