#!/bin/bash
set -ev
cmake -DCMAKE_BUILD_TYPE=debug .
make
make tests
