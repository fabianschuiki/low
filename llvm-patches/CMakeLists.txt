# Copyright (c) 2015 Fabian Schuiki, Thomas Richner
add_library(llvm-patches STATIC
	llvm_intrinsics.cpp
)

set_target_properties(llvm-patches PROPERTIES
	LINKER_LANGUAGE "CXX"
	COMPILE_FLAGS "${LLVM_COMPILE_FLAGS} -std=c++11"
	LINK_FLAGS ${LLVM_LINK_FLAGS}
)
