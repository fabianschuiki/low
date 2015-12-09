#!/bin/bash
set -ev

# g++4.8
if [ "$CXX" = "g++" ]; then export CXX="g++-4.8"; fi
# clang 3.6
if [ "$CXX" == "clang++" ]; then export CXX="clang++-3.6"; fi

cmake -DCMAKE_BUILD_TYPE=debug .
make
make tests
