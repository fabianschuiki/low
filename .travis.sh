#!/bin/bash
set -ev

echo "C++ Compiler is: $CXX"

# g++4.8
if [ "$CC" = "gcc" ]; then export CXX="g++-4.8"; fi
# clang 3.6
if [ "$CC" == "clang" ]; then export CXX="clang++-3.6"; fi

cmake -DCMAKE_BUILD_TYPE=debug .
make
make tests
